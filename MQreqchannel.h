
#ifndef _MQreqchannel_H_
#define _MQreqchannel_H_

#include "common.h"
#include "reqchannel.h"

class MQRequestChannel: public RequestChannel
{
private:
	int bufcap;

public:
	MQRequestChannel(const string _name, const Side _side, int _bufcap);
	~MQRequestChannel();
	int cread (void* msgbuf, int bufcapacity);
	int cwrite (void *msgbuf , int msglen);
	int open_ipc(string _ipc_name, int mode);
};

#endif
