#!/usr/bin/env python
# _*_ coding:utf-8 _*_
import requests
import datetime
import json
import sys
import xlrd
import os
from xlrd import open_workbook
from xlutils.copy import copy
import xlwt

reload(sys)
sys.setdefaultencoding('utf-8')


def test(wav_url, text):
    start = datetime.datetime.now()
    print(start)
    my_url = 'http://192.168.1.101:8990/compute'
    # my_url = 'http://192.168.1.101:8990/GetSpeechScore'
    # my_url = 'http://asr.langlib.io/compute'
    # my_url = 'http://192.168.1.157:9902/compute'
    data = {
        # "wav_url": wav_url,
        # "reference_text": text,
        # "wav_time": 0,

        "wav-in-filename": wav_url,
        "reference-text": text

        }

    header = {'content-type': 'application/json'}
    r = requests.post(url=my_url, data=json.dumps(data), headers=header)
    end = datetime.datetime.now()
    print("时间差: ")
    print(end-start)
    return r.text

sentence_list = ["Their [s]preferred[/s] [s]habitat[/s] [-]is [s]forests[/s] [-][s]near[/s] [-]the [s]edge[/s] of [s]streams[/s]. [-][s]However[/s], [-][s]blackcaps[/s] [-][s]also[/s] [s]live[/s] [-]in [s]pine[/s] [s]woods[/s] [-][s]away[/s] from [s]water[/s].",
                 "It [s]turned[/s] [s]out[/s] [-]that [-]there were [-][s]actually[/s] [-][s]four[/s] [s]times[/s] [-]as [s]many[/s] [s]bird[/s] [s]pairs[/s] or [s]couples[/s] [s]living[/s] [-]in the [s]stream[/s] [s]edge[/s] [s]habitat[/s] [-][s]compared[/s] to [-]the [s]area[/s] [-][s]away[/s] from the [s]stream[/s]."]

myWorkbook = open_workbook("data.xlsx") # 打开原有excel
mySheets = myWorkbook.sheets()         # 获取工作表list。
mySheet = mySheets[0]                  # 通过索引顺序获取。

nrows = mySheet.nrows
ncols = mySheet.ncols

print(nrows)
print(ncols)

# 新建一个excel文件
file = xlwt.Workbook(encoding = 'utf-8')
# 新建一个sheet
table = file.add_sheet('info', cell_overwrite_ok=True)
# 写入数据table.write(行,列,value)
table.write(0, 0, "用户ID")
table.write(0, 1, "音频名称")
table.write(0, 3, "句子评分")
table.write(0, 5, "停顿评分")
table.write(0, 7, "重音评分")

wav_index = 1

for i in range(1, nrows):
    user_id = mySheet.cell_value(i, 2)

    # excel_sentence = mySheet.cell_value(i, 5)

    if (i % 2) == 0:
        excel_sentence = sentence_list[1]
    else:
        excel_sentence = sentence_list[0]

    print(excel_sentence)

    excel_url = mySheet.cell_value(i, 6)
    excel_url = "https://ssl-public.langlib.com/" + excel_url

    basic_name = os.path.basename(excel_url)
    wav_file_name = basic_name[-10:-3] + str("wav")

    dic_str = test(excel_url, excel_sentence)
    # print(excel_sentence)
    # print(excel_url)
    print(dic_str)

    dic = eval(dic_str)

    # if i == 1:
    #    break

    # if dic.get("error_no") == 0:
    table.write((wav_index + 1), 0, str(user_id))
    table.write((wav_index + 1), 1, str(wav_file_name))

    table.write(wav_index, 2, str("基本方案"))

    table.write(wav_index, 3, str(dic.get("pronunciation_score")))
    table.write(wav_index, 5, str(dic.get("basic_pause_score")))
    table.write(wav_index, 7, str(dic.get("basic_stress_score")))

    table.write((wav_index+1), 2, str("方案2"))
    table.write((wav_index+1), 3, str(dic.get("pronunciation_score")))
    table.write((wav_index+1), 5, str(dic.get("project2_pause_score")))
    table.write((wav_index+1), 7, str(dic.get("project2_stress_score")))

    table.write((wav_index + 2), 2, str("方案3"))
    table.write((wav_index + 2), 3, str(dic.get("pronunciation_score")))
    table.write((wav_index + 2), 5, str(dic.get("project3_pause_score")))
    table.write((wav_index + 2), 7, str(dic.get("project3_stress_score")))

    table.write((wav_index + 3), 2, str("方案4"))
    table.write((wav_index + 3), 3, str(dic.get("pronunciation_score")))
    table.write((wav_index + 3), 5, str(dic.get("project4_pause_score")))
    table.write((wav_index + 3), 7, str(dic.get("project4_stress_score")))

    wav_index = wav_index + 5

    # print(r.text)
    # write_new_excel(r, i)


file.save('file.xls')

