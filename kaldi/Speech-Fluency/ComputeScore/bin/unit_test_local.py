#!/usr/bin/env python
# _*_ coding:utf-8 _*_
import requests
import datetime
import json
import xlwt
import xlrd
import sys
from xlrd import open_workbook
from xlutils.copy import copy

reload(sys)
sys.setdefaultencoding('utf-8')

def test(wav_file_name, wav_text):

    my_url = 'http://192.168.1.101:8990/ComputeTest'
    #my_url = 'http://127.0.0.1:9995/compute'

    data = {
        "wav-in-filename": wav_file_name,
        "reference-text": wav_text
    }
    header = {'content-type': 'application/json'}
    r = requests.post(url=my_url,data=json.dumps(data),headers=header)

    #print(r.url)
    return r.text

def test2():
    start = datetime.datetime.now()
    print(start)
    my_url = 'http://192.168.1.101:8990/ComputeTest'
    data = {
        "wav-in-filename": "1.wav",
        #"reference-text": "Their [s]preferred[/s] [s]habitat[/s][-]is [s]forests[/s][s]near[/s] the [s]edge[/s] of [s]streams[/s].[-][s]However[/s],[-][s]blackcaps[/s] [s]also[/s] [s]live[/s] in [s]pine[/s] [s]woods[/s][-][s]away[/s][-]from [s]water[/s]."
        #"reference-text": "good morning miss"
        "reference-text": "Crying helps me slow down and obsess over the weight of life's problem"
    }

    header = {'content-type': 'application/json'}
    r = requests.post(url=my_url, data=json.dumps(data), headers=header)

    end = datetime.datetime.now()
    print("时间差: ")
    print(end - start)
    print(r.text)

list = ["What's the weather like in spring in Guangzhou",
         "There are fourty-eight students in our class . What about your class ? ",
         "Wow you look great in that new shirt !",
         "I often stay up late . How many hours do you sleep ? ",
         "Excuse me , how can I go to the shopping center from here ? ",
         "I always have a lot of homework . How many hours do you spend on your homework ? ",
         "You work so hard . What do you do for fun ?",
         "Your notebook looks nice . Where did you buy it ?",
         "Lunch is too expensive in my school . How much do you spend for your lunch? ",
         "My mother is very ill . She can't go to work ."
        ]
#"[s]Millions[/s] [s]tuned[/s] [s]in[/s] [s]to[/s] [s]watch[/s] the [s]historic[/s] [s]moment[/s] [-] on [s]television[/s] [s]around[/s] the [s]world[/s].",
list2 = [
        "And[-]if the [s]seeds[/s] [s]land[/s][-]in a [s]suitable[/s] [s]habitat[/s],[-]they [s]do[/s] [s]well[/s][-]and [s]reproduce[/s].[-]With [s]active[/s] [s]habitat's[/s] [s]selection[/s],[-]an [s]organism[/s] is [s]able[/s] to [s]physically[/s] [s]select[/s][-][s]where[/s] to [s]live[/s][-]and [s]breed[/s].",
        "Their [s]preferred[/s] [s]habitat[/s][-]is [s]forests[/s][s]near[/s] the [s]edge[/s] of [s]streams[/s].[-][s]However[/s],[-][s]blackcaps[/s] [s]also[/s] [s]live[/s] in [s]pine[/s] [s]woods[/s][-][s]away[/s][-]from [s]water[/s].",
        "[s]Studies[/s] have been [s]done[/s] on the [s]reproductive[/s] [s]success[/s] [s]rates[/s][-]for the [s]birds[/s] in [s]both[/s] [s]areas[/s],[-]and the [s]result[/s] [s]showed[/s][-][s]surprisingly[/s][-][s]that[/s][-]the [s]reproductive[/s] [s]success[/s] was [s]essentially[/s] the [s]same[/s][-]in [s]both[/s] [s]areas[/s][-] he [s]preferred[/s][-][s]and[/s][-]the [s]second[/s] [s]choice[/s] [s]habitat[/s].",
        "It [s]turned[/s] [s]out[/s][-]that there were [s]actually[/s] [s]four[/s] [s]times[/s] as [s]many[/s] [s]bird[/s] [s]pairs[/s][-]or [s]couples[/s][-][s]living[/s] in the [s]stream[/s] [s]edge[/s] [s]habitat[/s][-][s]compared[/s] to the [s]area[/s][-][s]away[/s] from the [s]stream[/s].",
        "But it [s]lays[/s] its [s]eggs[/s] in [s]shallow[/s] [s]depressions[/s] in the [s]sand[/s][-]with [s]very[/s] [s]little[/s] [s]protection[/s] [s]around[/s] them.[-][s]So[/s] if there are [s]people[/s] or [s]dogs[/s] on the [s]beach[/s],[-]the [s]eggs[/s] and [s]fledglings[/s] in the [s]nests[/s][-]are [s]really[/s] [s]vulnerable[/s].",
        "[s]Three[/s] [s]Chinese[/s] [s]astronauts[/s] [s]returned[/s] [s]safely[/s] on [s]Sunday[/s] [-] after [s]orbiting[/s] the [s]Earth[/s] [-] on [s]board[/s] the [s]Shengzhou-VII[/s] [s]space[/s] [s]capsule[/s].",
        "The [s]message[/s] to the [s]world[/s] is [s]clear[/s]: [-] after a [s]slow[/s] [s]start[/s] in [s]space[/s], [-][s]China[/s] is [s]catching[/s] up [s]fast[/s].",
        "The [s]spacecraft[/s] [s]touched[/s] [s]down[/s] in the [s]Mongolian[/s] [s]desert[/s] [-] to [s]rapturous[/s] [s]applause[/s] from [s]mission[/s] [s]control[/s] in [s]Beijing[/s].",
        "Millions [s]tuned[/s] [s]in[/s] to watch the historic moment [-] on television around the world.",
        "[s]Mission[/s] [s]leader[/s] [-] [s]Zhai[/s] [s]Zhigang[/s] [-] was the [s]first[/s] to [s]emerge[/s] [s]from[/s] the [s]capsule[/s]. [-]He was [s]incredibly[/s] [s]proud[/s] of their [s]achievement[/s]."
        ]
list3 = ["caonaifan","chenyuan","cuibeibei","dailu","duchuan","fengli","gouyan",
         "haoxiaoling","houlili","jianglu","jiangxin","jincongcong","jinhao",
         "leikaiyu","Libeijia","lidandan","LiLiwen","litian","liuhan","liuqi","liuxiaoli",
         "liuyuewei","liyuting","peiqianru","shijiayue","tanweixiao","wanghaiqiao","wangmingming",
         "wangweichao","wangyao","wangyihan","wangyu","weilai","wuqianwen","zhangmei","zhangyan",
         "Zhujinghan"
        ]
list4 = ["dinghaojie","fengmeng","jianglu","lidonghua","liusimeng","liuyunming","miaoyunyun",
         "zhangwy","zhouming","zhudaolu"
        ]

list5 = ["jianglu"]

"""
for e in list5:
    #name = '../../dep/langbo_data/wangmingming/'
    name = "../../dep/langbo_data/"
    #name = '../../dep/temp_end/'
    name =name + e +"/"
    print(name)

    data = open_workbook("score.xls") #打开原有excel
    old_gop_rows = data.sheets()[0].nrows #获取原有的行数
    old_stress_rows = data.sheets()[1].nrows
    old_pause_rows = data.sheets()[2].nrows
    old_word_stree_rows = data.sheets()[3].nrows
    old_vowel_score_rows = data.sheets()[4].nrows      #元音
    old_consonant_score_rows = data.sheets()[5].nrows  #辅音

    new_data = copy(data)
    table_gop = new_data.get_sheet(0)
    table_stress = new_data.get_sheet(1)
    table_pause = new_data.get_sheet(2)
    table_word_stree_rows = new_data.get_sheet(3)
    table_vowel = new_data.get_sheet(4)
    table_consonant = new_data.get_sheet(5)

    table_gop.write(old_gop_rows, 0, label=name[22:-1])
    table_stress.write(old_stress_rows, 0, label=name[22:-1])
    table_pause.write(old_pause_rows, 0, label=name[22:-1])
    table_word_stree_rows.write(old_word_stree_rows, 0, label=name[22:-1])
    table_vowel.write(old_vowel_score_rows, 0, label=name[22:-1])
    table_consonant.write(old_consonant_score_rows, 0, label=name[22:-1])

    for i in range(1,2):
        now = datetime.datetime.now()
        print("***********************time start %d*******************"%i)
        print(now.strftime('%Y-%m-%d %H:%M:%S'))

        wav = name+str(i)+".wav"
        wav_text = list2[i-1]
        #wav_text = list[i - 1]
        r = test(wav, wav_text)


        if len(r) > 3:
            print(r)
            rs = json.loads(r)
            print(rs)

            table_gop.write(old_gop_rows, i, label=int(rs.get("data").get("vowel_score")))
            table_stress.write(old_stress_rows, i, label=rs.get("data").get("stress_score"))
            table_pause.write(old_pause_rows, i, label=rs.get("data").get("pause_score"))
            table_vowel.write(old_vowel_score_rows, i, label=rs.get("data").get("vowel_score"))
            table_consonant.write(old_consonant_score_rows, i, label=rs.get("data").get("consonant_score"))
            table_word_stree_rows.write(old_word_stree_rows, i, label=rs.get("data").get("words_stress_score"))

        else:
            table_gop.write(old_gop_rows, i, 0)
            table_stress.write(old_stress_rows, i, 0)
            table_pause.write(old_pause_rows, i, 0)
            table_vowel.write(old_vowel_score_rows, i, 0)
            table_consonant.write(old_consonant_score_rows, i, 0)

            table_word_stree_rows.write(old_word_stree_rows, i, 0)

        now = datetime.datetime.now()
        print(now.strftime('%Y-%m-%d %H:%M:%S'))
        print("**********************time end %d**********************"%i)
    new_data.save('score.xls')
"""

test2()
