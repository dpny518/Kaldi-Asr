
# coding: utf-8

# In[1]:


import requests
import sys
import json


reload(sys)
sys.setdefaultencoding('utf-8')

def test_zhangwy():

    url='http://192.168.187.146:8990/fluency'

    #my_params = dict()



    my_params = json.dumps({
        "compare_sentence": "[s]one[/s] of [s]my[/s] [s]students[/s]  [s]just[/s] [s]dropped[/s] [s]out[/s] of the [s]field[/s] [s]trip[/s] to the [s]Smithson[/s] [s]River[/s] [s]Caves[/s]. [s]You’re[/s] [s]next[/s] on the [s]waiting[/s] [s]list[/s], [-][s]so[/s] [s]now[/s] [s]there’s[/s] [s]room[/s] for you to [s]come[/s] [s]along[/s].",
        "wav_in_file_path": "http://7xjy77.com2.z0.glb.qiniucdn.com/1301/sentence/33348.amr"
    })

    headers = {'content-type': 'application/json'}

    #my_params = json.dumps({'wav_in_file_path': 'testwav/tt09.wav', 'compare_sentence': 'one of my students just dropped out of the field trip to the Smithson River Caves. You\'re next on the waiting list, so now there\'s room for you to come along.'})

   # r = requests.post(url=my_url, data=my_params)

    r = requests.post(url, data=my_params, headers=headers)

# r = requests.post(url=my_url, data=json.dumps(my_params))

    print(r.url)

    print(r.text)


test_zhangwy()

