#include "Functions.h"

bool Recv(_ClientInfo* _ptr)
{
	int retval;
	DWORD recvbytes;
	DWORD flag = 0;

	ZeroMemory(&_ptr->RecvPacket.r_overlapped.overlapped, sizeof(WSAOVERLAPPED));
	_ptr->RecvPacket.r_wsabuf.buf = _ptr->RecvPacket.recvbuf + _ptr->RecvPacket.comp_recvbytes;
	_ptr->RecvPacket.r_wsabuf.len = _ptr->RecvPacket.recvbytes - _ptr->RecvPacket.comp_recvbytes;

	retval = WSARecv(_ptr->sock, &_ptr->RecvPacket.r_wsabuf, 1, &recvbytes,
		&flag, &_ptr->RecvPacket.r_overlapped.overlapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			err_display("WSARecv()");
			return false;
		}
	}
	return true;
}

bool Send(_ClientInfo* _ptr, char* _buf = nullptr, int _size = 0)
{
	int retval;
	DWORD sendbytes;
	DWORD flag = 0;

	_SendPacket* packet = nullptr;

	if (_buf != nullptr && _size != 0)
	{
		packet = new _SendPacket();

		ZeroMemory(packet, sizeof(_SendPacket));

		memcpy(packet->sendbuf, _buf, _size);
		packet->sendbytes = _size;
		packet->s_overlapped.ptr = _ptr;
		packet->s_overlapped.iotype = IO_TYPE::IO_SEND;

		_ptr->SendQueue->push(packet);
		if (_ptr->SendQueue->size() > 1)
		{
			return true;
		}
	}

	packet = _ptr->SendQueue->front();

	ZeroMemory(&packet->s_overlapped.overlapped, sizeof(WSAOVERLAPPED));
	packet->s_wsabuf.buf = packet->sendbuf + packet->comp_sendbytes;
	packet->s_wsabuf.len = packet->sendbytes - packet->comp_sendbytes;

	retval = WSASend(_ptr->sock, &packet->s_wsabuf, 1, &sendbytes,
		flag, &packet->s_overlapped.overlapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			err_display("WSASend()");
			return false;
		}
	}
	return true;
}

int CompleteRecv(_ClientInfo* _ptr, int cbTransferred)
{
	_ptr->RecvPacket.comp_recvbytes += cbTransferred;
	if (_ptr->RecvPacket.recvbytes == _ptr->RecvPacket.comp_recvbytes)
	{
		_ptr->RecvPacket.comp_recvbytes = 0;

		if (_ptr->RecvPacket.r_sizeflag)
		{
			_ptr->RecvPacket.r_sizeflag = false;
			_ptr->RecvPacket.recvbytes = sizeof(int);
			return SOC_TRUE;
		}
		else
		{
			_ptr->RecvPacket.r_sizeflag = true;
			memcpy(&(_ptr->RecvPacket.recvbytes), _ptr->RecvPacket.r_wsabuf.buf, sizeof(int));
		}
	}
	if (!Recv(_ptr))
	{
		RemoveClientInfo(_ptr);
		return SOC_ERROR;
	}
	return SOC_FALSE;
}

int CompleteSend(_ClientInfo* _ptr, int cbTransferred)
{
	_SendPacket* packet = _ptr->SendQueue->front();

	packet->comp_sendbytes += cbTransferred;
	if (packet->sendbytes == packet->comp_sendbytes)
	{
		_ptr->SendQueue->pop();
		delete packet;

		if (_ptr->SendQueue->size() > 0)
		{
			Send(_ptr);
		}

		return SOC_TRUE;
	}
	if (!Send(_ptr))
	{
		return SOC_ERROR;
	}
	return SOC_FALSE;
}
