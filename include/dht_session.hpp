#ifndef BITTORRENT_DHT_SESSION_HPP
#define BITTORRENT_DHT_SESSION_HPP

#include <thread>
#include <future>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <dht.h>
#include <ExternalIPCounter.h>
#include <libminiupnpc/igd_desc_parse.h>
#include "udp_socket.hpp"
#include "scout.hpp"

namespace scout
{

struct upnp_mapping
{
	upnp_mapping(char* ctrlURL, char st[MINIUPNPC_URL_MAXSIZE])
		: controlURL(ctrlURL)
	{
		memcpy(servicetype, st, sizeof(servicetype));
	}

	std::string controlURL;
	char servicetype[MINIUPNPC_URL_MAXSIZE];
};

class dht_session
{
	friend struct ip_change_observer_session;

public:
	enum run_state
	{
		INITIAL,
		RUNNING,
		QUITTING,
	};

	dht_session();
	~dht_session();

	int start();
	void stop();

	// synchronize a list of entries with the DHT
	// this will first update the given vector with any new or updated entries from the DHT
	// then store the updated list in the DHT
	void synchronize(secret_key_span shared_key, std::vector<entry> const& entries
		, entry_updated entry_cb, finalize_entries finalize_cb, sync_finished finished_cb);

	// store an immutable item in the DHT
	void put(list_token const& token, gsl::span<gsl::byte const> contents
		, put_finished finished_cb);

	// retrieve an immutable item from the DHT
	void get(hash_span address, item_received received_cb);

private:
	bool is_quitting() const { return m_state == QUITTING; }
	void resolve_bootstrap_servers();
	void update_mappings();
	void network_thread_fun(std::promise<int>& promise);
	void on_dht_timer(error_code const& ec);
	void on_natpmp_timer(error_code const& ec);
	void on_ip_changed(udp::endpoint const& new_ip);
	void incoming_packet(char* buf, size_t len, udp::endpoint const& ep);

	boost::asio::io_service m_ios;
	std::uint16_t m_dht_external_port;
	run_state m_state;
	ExternalIPCounter m_external_ip;
	std::shared_ptr<udp_socket> m_socket;
	std::thread m_thread;
	smart_ptr<IDht> m_dht;
	boost::asio::steady_timer m_dht_timer;
	// io service for handling various async tasks (such as nat-pmp):
	io_service m_ios_worker;
	// worker thread used by the worker io service:
	std::thread m_worker_thread;
	boost::asio::steady_timer m_natpmp_timer;
	std::vector<upnp_mapping> m_upnp_mappings;
	bool m_is_natpmp_mapped;
	std::vector<std::pair<std::string, int>> m_bootstrap_nodes;
	int m_dht_rate_limit;
};

} // namespace scout

#endif
