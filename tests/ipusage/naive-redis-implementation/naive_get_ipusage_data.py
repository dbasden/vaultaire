#!/usr/bin/env python
'''firstly, an absolutely naieve implementation using redis
'''

import redis
from itertools import *

import datetime

def fetch_ip_data(ip_address, redis_con):
    # here we would normally get the ip usage data for the IP and chuck it into redis
    # for this proof of concept, it's already there.
    pass


def reduce_ip_data(ip_address, redis_con, direction='rx', seconds_per_group=600):
    '''get all data points for an IP address and group them into blocks of a specific size'''
    source = 'ip_capture:traffic:%s:%s_bytes' % (ip_address,direction)
    zindex = 'by_timestamp:'+source

    timestamps = redis_con.zrange(zindex, 0, -1)
    vals = redis_con.hmget(source, timestamps)

    traffic_data = izip(imap(int, timestamps), imap(int, vals))

    nanoseconds_per_group = seconds_per_group * 10**9
    group_predicate = lambda (t,v): t / nanoseconds_per_group

    for group_key,group in groupby(traffic_data, group_predicate):
        group_key *= nanoseconds_per_group
        group = list(group)
        total = sum(v for k,v in group)
        yield group_key, total


def ip_usage_to_json(ip_address,rxusage, txusage):
    '''sample output for data'''
    import json

    outdata = { 
            ip_address: {
                'bytes_inbound': 
                    [dict(timestamp=(t//10**9),bytes=b) for t,b in rxdata],
                'bytes_outbound': 
                    [dict(timestamp=(t//10**9),bytes=b) for t,b in txdata]
            }
        }

    return json.dumps(outdata, sys.stdout)

def ip_usage_to_txt(rxusage, txusage):
    '''sample output for data'''

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print >> sys.stderr, sys.argv[0],  "<ip address>"
        sys.exit(1)
    ip_address = sys.argv[1]

    redis_con = redis.StrictRedis(host='localhost', port=6379,db=0)
    fetch_ip_data(ip_address, redis_con)
    rxdata = reduce_ip_data(ip_address, redis_con, direction='rx')
    txdata = reduce_ip_data(ip_address, redis_con, direction='tx')

    # throw it out in a less obscure form now we don't have
    # to be all about the performance

    #print ip_usage_to_json(ip_address, rxdata, txdata)

    # Or to just be pretty we could use gnuplot to output it.
    print r'''#!/usr/bin/env gnuplot


    set terminal png
    set output "'''+ip_address+'''ip_usage.png"

    set xdata time
    set timefmt "%s"
    plot '-' using 1:2 index 0 with boxes title "received bytes"
    plot '-' using 1:2 index 1 with boxes title "sent bytes"'''

    print '# Received Bytes'

    for timestamp,bytecount in rxdata:
        print timestamp // 10**9, bytecount

    print
    print
    print '# Sent Bytes'

    for timestamp,bytecount in txdata:
        print timestamp // 10**9, bytecount
