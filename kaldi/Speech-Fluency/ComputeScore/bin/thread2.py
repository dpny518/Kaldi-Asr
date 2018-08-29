# _*_ coding:utf-8 _*_
import requests
import datetime
import json
import sys
import threading
from threading import Thread
import time
import gevent

reload(sys)
sys.setdefaultencoding('utf-8')

# def sayhi(name):
#     print('%s say hello' %name)
#
#
# if __name__ == '__main__':
#     t = Thread(target=sayhi, args=('xiaoming'))
#     t.start()
#     print('主线程')

def test():
    start = datetime.datetime.now()
    print(start)
    my_url = 'http://192.168.1.101:8800/compute'
    #my_url = 'http://192.168.1.101:8800/GetSpeechScore'
    # my_url = 'http://asr.langlib.io/compute'
    # my_url = 'http://192.168.1.157:9902/GetSpeechScore'

    wav_url = "http://7xjy77.com2.z0.glb.qiniucdn.com/1301/sentence/33348.amr"
    text = "History [s]proves[/s] that it doesn\'t [s]matter[/s] whether [-] you [s]come[/s] from a council [s]estate[/s] or a country estate"
    data = {
        #"wav_url": wav_url,
        #"reference_text": text,
        #"wav_time": 6000,

        "wav-in-filename": wav_url,
        "reference-text": text

    }

    header = {'content-type': 'application/json'}
    r = requests.post(url=my_url, data=json.dumps(data), headers=header)
    end = datetime.datetime.now()
    print("时间差: ")
    print(end - start)
    # rs = json.loads(r.text)
    # print(rs)
    print(r.text)
    return r.text

class MyThread(threading.Thread):
    def __init__(self,arg):
        super(MyThread, self).__init__()
        self.arg=arg
    def run(self):
        test()
        print('the arg is:%s\r' % self.arg)

for i in xrange(5):
    t =MyThread(i)
    t.start()

print('main thread end!')
