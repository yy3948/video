import psutil
from psutil._common import bytes2human

#获取磁盘剩余空间和总空间
def get_disk_space(path='/'):
    usage = psutil.disk_usage(path)
    # print(usage)
    space_total = bytes2human(usage.total)
    space_used = bytes2human(usage.used)
    space_free = bytes2human(usage.free)
    space_used_percent = float(bytes2human(usage.percent)[:-1]) / 100
    # print('{:.2%} : {}/{}, remaining capacity {}'.format(space_used_percent,space_used,space_total,space_free))
    #输出剩余磁盘空间/总空间
    #print('{}/{}'.format(space_free, space_total))
    disk_info='free:'+'{}/{}'.format(space_free, space_total)
    per= "used:"+str(usage[3])+"%"
    print(disk_info, per)
    return  disk_info

#get_disk_space("D:")
