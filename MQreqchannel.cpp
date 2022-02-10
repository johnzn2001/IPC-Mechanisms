#include "common.h"
#include "MQreqchannel.h"
#include <mqueue.h>
using namespace std;

MQRequestChannel::MQRequestChannel(const string _name, const Side _side, int _bufcap) : RequestChannel(_name, _side) {
	s1 = "/MQ_" + my_name + "1";
	s2 = "/MQ_" + my_name + "2";
	bufcap = _bufcap;
		
	if (_side == SERVER_SIDE){
		wfd = open_ipc(s1, O_RDWR | O_CREAT);
		rfd = open_ipc(s2, O_RDWR | O_CREAT);
	}
	else{
		rfd = open_ipc(s1, O_RDWR | O_CREAT);
		wfd = open_ipc(s2, O_RDWR | O_CREAT);
	}
	
}

MQRequestChannel::~MQRequestChannel(){ 
	mq_close(wfd);
	mq_close(rfd);

	mq_unlink(s1.c_str());
	mq_unlink(s2.c_str());
}

int MQRequestChannel::open_ipc(string _pipe_name, int mode){
	mq_attr attr {0, 1, bufcap, 0};
	int fd = (int) mq_open(_pipe_name.c_str(), O_RDWR | O_CREAT, 0600, &attr);
	if (fd < 0){
		EXITONERROR(_pipe_name);
	}
	return fd;
}

int MQRequestChannel::cread(void* msgbuf, int bufcapacity){
	return mq_receive (rfd, (char*) msgbuf, 8192, NULL);
}

int MQRequestChannel::cwrite(void* msgbuf, int len){
	return mq_send (wfd, (char*) msgbuf, len, NULL);
}

