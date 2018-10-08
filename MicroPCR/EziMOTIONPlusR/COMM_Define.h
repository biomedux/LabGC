
#pragma once

//-------------------------Constants------------------------------------------ 

static const int	MAX_SLAVE_NUMS = 20;

static const int	MAX_BUFFER_SIZE = 256;		///< Maximum Send/Receive Packet Size
static const int	MAX_SWINFO_LENGTH = 248;

static const int	COMM_WAITTIME = 100;

static const int	COMM_TIME_STATUSREFRESH = 100;

static const int	COMM_TIME_IDLE = (/*COMM_TIME_WAITING*/500 / COMM_TIME_STATUSREFRESH);

static const BYTE	MAX_ALLOWED_TIMEOUT = 6;