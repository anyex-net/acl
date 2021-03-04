#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_ack.hpp"

namespace acl {

enum {
	MQTT_STAT_HDR_VAR,
};

mqtt_ack::mqtt_ack(mqtt_type_t type)
: mqtt_message(type)
, finished_(false)
, hlen_(0)
, pkt_id_(0)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_ack::~mqtt_ack(void) {}

void mqtt_ack::set_pkt_id(unsigned short id) {
	pkt_id_ = id;
}

bool mqtt_ack::to_string(string& out) {
	bool old_mode = out.get_bin();

	this->set_data_length(2);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	this->pack_add((unsigned short) pkt_id_, out);

	out.set_bin(old_mode);
	return true;
}

static struct {
	int status;
	int (mqtt_ack::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_VAR,	&mqtt_ack::update_header_var		},
};

int mqtt_ack::update(const char* data, int dlen) {
	if (data == NULL || dlen  <= 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		data += dlen - ret;
		dlen  = ret;
	}
	return dlen;
}

#define	HDR_VAR_LEN	2

int mqtt_ack::update_header_var(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(hbuf_) >= HDR_VAR_LEN);

	if (hlen_ >= HDR_VAR_LEN) {
		logger_error("invalid header var");
		return -1;
	}

	for (; hlen_ < HDR_VAR_LEN && dlen > 0;) {
		hbuf_[hlen_++] = *data++;
		dlen --;
	}

	if (hlen_ < HDR_VAR_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	if (!this->unpack_short(&hbuf_[0], 2, pkt_id_)) {
		logger_error("unpack pkt id error");
		return -1;
	}
	finished_ = true;
	return dlen;
}

} // namespace acl
