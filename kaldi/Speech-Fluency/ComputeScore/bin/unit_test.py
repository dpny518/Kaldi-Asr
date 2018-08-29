#!/usr/bin/env python
# _*_ coding:utf-8 _*_
import requests
import datetime
import json

import sys
import xlrd
import xlwt

# import requests.packages.urllib3.util.ssl_

# requests.packages.urllib3.util.ssl_.DEFAULT_CIPHERS = 'ALL'

reload(sys)
sys.setdefaultencoding('utf-8')


def test(wav_url, text):
    start = datetime.datetime.now()
    print(start)
    my_url = 'http://192.168.1.101:8990/compute'
    #my_url = 'http://192.168.1.101:8990/GetSpeechScore'
    # my_url = 'http://asr.langlib.io/compute'
    # my_url = 'http://192.168.1.157:9902/compute'
    data = {
        #"wav_url": wav_url,
        #"reference_text": text,
        #"wav_time": 0,

        "wav-in-filename": wav_url,
        "reference-text": text

        }

    header = {'content-type': 'application/json'}
    r = requests.post(url=my_url, data=json.dumps(data), headers=header)
    end = datetime.datetime.now()
    print("时间差: ")
    print(end-start)
    return r.text

list2 = [
    "It [s]turned[/s] [s]out[/s][-]that there were [s]actually[/s] [s]four[/s] [s]times[/s] as [s]many[/s] [s]bird[/s] [s]pairs[/s][-]or [s]couples[/s][-][s]living[/s] in the [s]stream[/s] [s]edge[/s] [s]habitat[/s][-][s]compared[/s] to the [s]area[/s][-][s]away[/s] from the [s]stream[/s]",
    "Their [s]preferred[/s] [s]habitat[/s][-]is [s]forests[/s][s]near[/s] the [s]edge[/s] of [s]streams[/s].[-][s]However[/s],[-][s]blackcaps[/s] [s]also[/s] [s]live[/s] in [s]pine[/s] [s]woods[/s][-][s]away[/s][-]from [s]water[/s]."
]

# url = "https://ssl-public.langlib.com/lcm/audios/prod/4/204002-02/514d7132f07c1b991dac2e6b3e03ad77-1530723688415.mp3"
# url = "https://ssl-public.langlib.com/pe/aha/test/user/1/1/record_1533525808274.mp3"
# text ="It [s]turned[/s] [s]out[/s][-]that there were [s]actually[/s] [s]four[/s] [s]times[/s] as [s]many[/s] [s]bird[/s] [s]pairs[/s][-]or [s]couples[/s][-][s]living[/s] in the [s]stream[/s] [s]edge[/s] [s]habitat[/s][-][s]compared[/s] to the [s]area[/s][-][s]away[/s] from the [s]stream[/s]"

url = "https://ssl-public.langlib.com/lcm/audios/prod/4/204001-01/eab9729a770d0cedb4d0c7dca85d1124-1530797545866.mp3"
text = "It [s]turned[/s] [s]out[/s] [-]that [-]there were [-][s]actually[/s] [-][s]four[/s] [s]times[/s] [-]as [s]many[/s] [s]bird[/s] [s]pairs[/s] or [s]couples[/s] [s]living[/s] [-]in the [s]stream[/s] [s]edge[/s] [s]habitat[/s] [-][s]compared[/s] to [-]the [s]area[/s] [-][s]away[/s] from the [s]stream[/s]."

r = test(url, text)

print(r)
