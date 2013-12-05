#!/usr/bin/env python

'''import frames into a redis database

adds a ZSET per source called by_timestamp:(source), with the timestamp as rank
and value

You could then do something like this:

eval "local arr={}; for i,k in ipairs(redis.call('zrangebyscore',KEYS[1],ARGV[1],ARGV[2])) do arr[i] = redis.call('hget',KEYS[2],k) end return arr" 2 by_timestamp:ip_capture:traffic:110.173.154.121:rx_bytes ip_capture:traffic:110.173.154.121:rx_bytes 1385961901096254000 1385961901675069000

'''

import sys
import struct
import redis

import dataframe

class FrameReader(object):
    '''iterate over frames from a stream that are delimited by uint32 length headers
    '''
    def __init__(self, source=sys.stdin, max_frame_length=10**6):
        self.source = sys.stdin
        self.max_frame_length = max_frame_length

    def __iter__(self):
        while True:
            head = self.source.read(4)
            if len(head) != 4: break

            to_read, = struct.unpack('>I', head)
            if to_read > self.max_frame_length:
                raise IOError("Got bogus frame length to read:",to_read,"bytes")

            buf = self.source.read(to_read)
            if len(buf) != to_read:
                raise IOError("Truncated frame read. Expected",to_read,"bytes" \
                        " from header. Only got",len(buf),"bytes")
            yield buf

class DataFrameReader(FrameReader):
    def __iter__(self):
        bufiter = super(DataFrameReader,self).__iter__()
        for buf in bufiter:
            df = dataframe.DataFrame()
            df.ParseFromString(buf)
            yield df


class RedisFrameWriter(object):
    '''write frames into redis
    Doesn't pay much attention to what the frames are'''
    def __init__(self, redis_connection):
        self._redis = redis_connection
    def add_frames(self, frames):
        for frame in frames:
            self._redis.hset(frame.source, frame.timestamp, frame.value)
            self._redis.zadd('by_timestamp:'+frame.source, frame.timestamp,
                    frame.timestamp)


if __name__ == '__main__':
    redisdb = redis.StrictRedis(host='localhost', port=6379, db=0)
    rediswriter = RedisFrameWriter(redisdb)
    rediswriter.add_frames( DataFrameReader() )
