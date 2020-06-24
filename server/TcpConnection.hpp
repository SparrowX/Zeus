#ifndef _TcpConnection_hpp_
#define _TcpConnection_hpp_

#include "common.h"
#include "ObjectPool.hpp"


class TcpSocket : public ObjectPool<TcpSocket, 10000> {
public:
	TcpSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_msgBuf, 0, RECV_BUFF_SIZE);
		_msgLastPos = 0;
		memset(_sendBuf, 0, SEND_BUFF_SIZE);
		_sendLastPos = 0;
		resetHeartbeat();
		resetSendBuf();
	}

	~TcpSocket() {
		close();
	}

	SOCKET sockfd() {
		return _sockfd;
	}

	char* msgBuf() {
		return _msgBuf;
	}

	int getLastPos() {
		return _msgLastPos;
	}

	void setLastPos(int pos) {
		_msgLastPos = pos;
	}

	// Send message (put into the send buffer)
	int send(Message *msg) {
		int ret = SOCKET_ERROR;
		int sendLength = msg->length;
		const char *sendData = (const char *)msg;

		while (true) {
			// Data reaches the limitation of send buffer
			if (_sendLastPos + sendLength >= SEND_BUFF_SIZE) {
				int copyLength = SEND_BUFF_SIZE - _sendLastPos;
				memcpy(_sendBuf + _sendLastPos, sendData, copyLength);
				sendData += copyLength;
				sendLength -= copyLength;
				// Send the entire buffer
				ret = ::send(_sockfd, _sendBuf, SEND_BUFF_SIZE, 0);
				_sendLastPos = 0;
				resetSendBuf();
				if (SOCKET_ERROR == ret) {
					return ret;
				}
			}
			else {
				// Copy into the send buffer
				memcpy(_sendBuf + _sendLastPos, sendData, sendLength);
				_sendLastPos += sendLength;
				return 0;
			}
		}

		return ret;
	}

	// Clear the buffer (send everything out)
	int clearBuffer() {
		int ret = SOCKET_ERROR;
		if (_sendLastPos > 0) {
			ret = ::send(_sockfd, _sendBuf, _sendLastPos, 0);
			_sendLastPos = 0;
		}
		return ret;
	}

	// Close socket
	void close() {
		if (_sockfd == INVALID_SOCKET) return;
#ifdef _WIN32
		closesocket(_sockfd);
#else
		::close(_sockfd);
#endif
		_sockfd = INVALID_SOCKET;
	}

	void resetHeartbeat() {
		_tHeartbeat = 0;
	}

	void resetSendBuf() {
		_tSendBuf = 0;
	}

	bool isAlive(time_t t) {
		_tHeartbeat += t;
		return CLIENT_DEAD_TIME < 0 || _tHeartbeat < CLIENT_DEAD_TIME;
	}

	bool canSend(time_t t) {
		_tSendBuf += t;
		return CLIENT_SEND_TIME > 0 && _tSendBuf > CLIENT_SEND_TIME;
	}

private:
	SOCKET _sockfd;	// socket fd_set			
	char _msgBuf[RECV_BUFF_SIZE];	// Message Buffer
	int _msgLastPos;	// Last position of the message buffer
	char _sendBuf[SEND_BUFF_SIZE];	// Send Buffer
	int _sendLastPos;	// Last position of the send buffer
	time_t _tHeartbeat;	// For heartbeat detection
	time_t _tSendBuf;	// For clearing send buffer
};

typedef std::shared_ptr<TcpSocket> TcpConnection;

#endif // !_TcpConnection_hpp_

