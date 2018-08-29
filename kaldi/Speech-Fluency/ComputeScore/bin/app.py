# _*_ coding:utf-8 _*_
import requests
import os.path
import time
import logging, sys
import libcompute
import subprocess
import wave
import sys
import stat
import json

import connexion
import logging
import re
import datetime
import random

from flask import request

reload(sys)
sys.setdefaultencoding('utf-8')

# create a file logger
logger = logging.getLogger("asr")
formatter = logging.Formatter('%(asctime)s %(levelname)-8s: %(message)s')

file_handler = logging.FileHandler("asr.log")
file_handler.setFormatter(formatter)

logger.addHandler(file_handler)

def download_file(url):
    try:
        start = time.time()
        logger.info("downloading " + url)
        r = requests.get(url,verify=False)
        end = time.time()
        logger.info("downloading time: " + str(end - start))
        if not r.status_code == 200:
            return False
        with open(os.path.basename(url), "wb") as code:
            code.write(r.content)
            return True
    except (requests.RequestException, IOError), e:
        logger.exception("%s" % e)
        return False

def get_file_content(filePath):
    with open(filePath, 'rb') as fp:
        return fp.read()

def wav_format_convert(download_wav_file):

    if os.path.exists(download_wav_file)==False:
        return False

    #convert_wav_file_name = "test.wav"

    convert_wav_file_name = str("wav/") + download_wav_file[-10:-3] + str("wav")
    print("convert_wav_file_name:", convert_wav_file_name)

    if os.path.exists(convert_wav_file_name):
        os.remove(convert_wav_file_name)

    p = subprocess.Popen(['./ffmpeg', '-i', download_wav_file, '-ar', '16000', '-ac', '1', convert_wav_file_name])
    p.communicate()

    #p = subprocess.Popen(['./rnnoise', convert_wav_file_name])
    #p.communicate()

    if os.path.exists(convert_wav_file_name)==False or 0 == os.path.getsize(convert_wav_file_name):
        return False

def ComputeScore_online(wav_url, wav_text, type):

    start = datetime.datetime.now()
    if wav_url.strip() != "":
        if download_file(wav_url) == False:
            logger.error("Invalid url for user audio.")
            return dict(err_no=-1, err_msg="Invalid url for user audio.")
    end = datetime.datetime.now()
    print("下载需要时间:")
    print(end-start)
    if wav_format_convert(os.path.basename(wav_url)) == False:
        logger.error("convert wav failed.")
        return dict(err_no=-1, err_msg="convert wav failed.")

    print(os.path.basename(wav_url))
    #wav_in_filename = 'test_out.wav'
    # wav_in_filename = 'test.wav'

    basic_name = os.path.basename(wav_url)

    wav_in_filename = str("wav/") + basic_name[-10:-3] + str("wav")

    print("wav_in_filename", wav_in_filename)

    if type == 0:
        cps.ComputeScore(wav_in_filename, wav_text,1)
        res = cps.GetAppJsonStrResult()

    if type == 1:
        cps.ComputeScore(wav_in_filename, wav_text, 2)
        res = cps.GetAppJsonStrResult()

    if os.path.exists(os.path.basename(wav_url)):
        os.remove(os.path.basename(wav_url))

    #if os.path.exists(wav_in_filename):
    #    os.remove(wav_in_filename)

    return res

#error_no -1 system error ; 1 user error ; 0 right
#always return whole score
def compute_common(type):
    global chain_score
    global normal_speed
    data = {
        "pronunciation_score" : -1,
        "stress_score"   : -1,
        "pause_score"    : -1,
        "vowel_score"    : -1,
        "consonant_score" : -1,
        "sentence_stress_score": -1,
        "words_stress_score": -1,
        "speech_speed_score":-1
    }

    rs = {"error_no": 0,
          "error_msg": "succeed",
          "data": data
    }

    fluency_info = {
        "SentenceStressScore": -1,
        "WordsStressScore": -1,
        "pauseScore": -1,
        "wordScore": -1,
    }

    para_info = request.get_json()
    normal_time = 0

    #request field wrong for speech_score
    if (type == 0) and (( 'wav-in-filename' not in para_info) or ( 'reference-text' not in para_info) or (para_info['wav-in-filename'].strip()=='' ) or (para_info['reference-text'].strip() == '')):
        rs['error_msg'] = "checkout the param"
        rs['error_no'] = -1
        logger.error("param is not right")
        return json.dumps(rs)

    if (type == 1) and (( 'wav_url' not in para_info) or ( 'reference_text' not in para_info) or (para_info['wav_url'].strip()=='' ) or (para_info['reference_text'].strip() == '')):
        rs['error_msg'] = "checkout the param"
        rs['error_no'] = -1
        logger.error("param is not right")
        return json.dumps(rs)
    #time field
    if "wav_time" in para_info and para_info['wav_time'] > 0:
        normal_time = int(para_info['wav_time'])/1000


    reference_text = ""
    wav_in_filename = ""
    #bad code
    if type == 1 :
        wav_in_filename =str(para_info['wav_url'])
        reference_text =str(para_info['reference_text'])
    elif type == 0:
        wav_in_filename = str(para_info['wav-in-filename'])
        reference_text = str(para_info['reference-text'])

    logger.info("wav_file_name: %s " %wav_in_filename)
    logger.info("wav_txt: %s " % reference_text)
    logger.info("wav_time: %s " % normal_time)

    wav_text = re.sub('[.,!?:]','',reference_text)
    temp_text = re.sub('\[.*?\]','',wav_text)

    list_text = temp_text.split(' ')
    #if no wav_time
    if normal_time == 0:
        normal_time = len(list_text)/normal_speed

    print("wav_text: %s" %wav_text)
    print("wav_file_name: %s" %wav_in_filename)
    print("wav_time: %d" %normal_time)
    r = ComputeScore_online(wav_in_filename, wav_text,type)

    if isinstance(r, dict):
        rs['error_msg'] = r['err_msg']
        rs['error_no'] = -1
        return json.dumps(rs)

    strlist = r.split('|')
    gop_info = json.loads(strlist[0])
    if strlist[1] != "":
        fluency_info = json.loads(strlist[1])
    else:
        fluency_info['SentenceStressScore'] = int(gop_info.get("SentenceStressScore"))
        fluency_info['pauseScore'] = int(gop_info.get("pauseScore"))
        fluency_info['WordsStressScore'] = int(gop_info.get("WordsStressScore"))
        fluency_info['wordScore'] = int(gop_info.get("wordScore"))
        json.dumps(fluency_info)

    data["pronunciation_score"] = int(gop_info.get("sentence_score"))
    data["vowel_score"] = int(gop_info.get("vowel_score"))
    data["consonant_score"] = int(gop_info.get("consonant_score"))

    data["stress_score"] = int(fluency_info.get("SentenceStressScore"))
    data["pause_score"] = int(fluency_info.get("pauseScore"))

    data["sentence_stress_score"] = int(fluency_info.get("SentenceStressScore"))
    data["words_stress_score"] = int(fluency_info.get("WordsStressScore"))

    real_time =  gop_info.get("wav_time")

    if (real_time >= int(normal_time)*0.8) and (real_time < int(normal_time)*1.2):
        data["speech_speed_score"] = random.randint(90,100)
    elif(
            (real_time < int(normal_time) * 0.8 and int(real_time) >= int(normal_time) * 0.5) or
            (real_time >= int(normal_time) * 1.2 and int(real_time) < int(normal_time) * 1.5)
        ):
        data["speech_speed_score"] = random.randint(80,90)
    elif(
            (real_time < int(normal_time) * 0.5 and int(real_time) >= int(normal_time) * 0.3) or
            (real_time >= int(normal_time) * 1.5 and int(real_time) < int(normal_time) * 1.8)
        ):
        data["speech_speed_score"] = random.randint(60, 80)
    elif (
            (real_time < int(normal_time) * 0.3 and int(real_time) >= int(normal_time) * 0.1) or
            (real_time >= int(normal_time) * 1.8 and int(real_time) < int(normal_time) * 2.0)
    ):
        data["speech_speed_score"] = random.randint(40, 60)
    else:
        data["speech_speed_score"] = random.randint(20, 40)


    #同时考虑gop和chain解码
    word_score = int(fluency_info.get("wordScore"))
    chain_score = word_score
    print("chain_score %d " %chain_score)

    """
    if word_score>gop_info.get("sentence_score"):
        if word_score >95:
            word_score = 95
        data["pronunciation_score"] = word_score
        data["vowel_score"] = word_score+5
        data["consonant_score"] = word_score

    #处理极端分数 小程序 实用英语 分数加权不一样所以阈值不一致
    print("type %d" %type)
    if word_score <= 10:
        data["stress_score"] = -1
        data["sentence_stress_score"] = -1
        data["pause_score"] = -1
        data["pronunciation_score"] = -1
        data["vowel_score"] = -1
        data["consonant_score"] = -1
        data["speech_speed_score"] = -1
        data["words_stress_score"] = -1

        rs['error_msg'] = "please say again!"
        rs['error_no'] = -1

        # 考虑没有重音标记且识别正确的情况
    if data["pause_score"] == -1 and data["pronunciation_score"] > 0 and word_score > 20:
        data["pause_score"] = (data["pronunciation_score"] + data["speech_speed_score"]) / 2

    if data["stress_score"] == -1 and data["pronunciation_score"] > 0 and word_score > 20:
        data["stress_score"] = (data["pronunciation_score"] + data["speech_speed_score"]) / 2

    #处理正常的地分数值
    if (word_score > 20) and (word_score <= 30):
        data["stress_score"] = word_score + random.randint(10, 15)
        data["sentence_stress_score"] = word_score + random.randint(10, 15)
        data["pause_score"] = word_score + random.randint(10, 15)
        data["pronunciation_score"] = word_score + random.randint(10, 15)
        data["vowel_score"] = word_score + random.randint(10, 15)
        data["consonant_score"] = word_score + random.randint(10, 15)
        data["words_stress_score"] = word_score + random.randint(10, 15)
    """
    # 均一化重音分数
    if (word_score < 10) and (data["vowel_score"] < 10) and (data["consonant_score"] < 10) and (word_score > 10) and data["vowel_score"] > 0:
        data["sentence_stress_score"] = data["vowel_score"] +random.randint(5, 10)
        data["words_stress_score"] = data["vowel_score"] +random.randint(5, 10)

    return json.dumps(rs)


def compute():
    rs = {
        "pronunciation_score": -1,
        "basic_stress_score": -1,
        "basic_pause_score": -1,
        "project2_pause_score": -1,
        "project2_stress_score": -1,
        "project3_pause_score": -1,
        "project3_stress_score": -1,
        "project4_pause_score": -1,
        "project4_stress_score": -1,
    }

    r = compute_common(0)
    jstr = json.loads(r)

    print(jstr)
    # logger.info("python result: %s " % jstr)

    pronunciation_score = jstr.get("data").get("pronunciation_score")
    basic_pause_score = jstr.get("data").get("pause_score")
    basic_stress_score = jstr.get("data").get("stress_score")

    print("pronunciation_score:", pronunciation_score)
    print("basic_pause_score:", basic_pause_score)
    print("basic_stress_score:", basic_stress_score)

    basic_pause_score = int(basic_pause_score / 100)

    # 方案2 3:7
    project2_pause_score = int(0.3*pronunciation_score + 0.7*basic_pause_score)
    project2_stress_score = int(0.3*pronunciation_score + 0.7*basic_stress_score)

    # 方案3 5:5
    project3_pause_score = int(0.5 * pronunciation_score + 0.5 * basic_pause_score)
    project3_stress_score = int(0.5 * pronunciation_score + 0.5 * basic_stress_score)

    # 方案4 7:3
    project4_pause_score = int(0.7 * pronunciation_score + 0.3 * basic_pause_score)
    project4_stress_score = int(0.7 * pronunciation_score + 0.3 * basic_stress_score)

    # rs['pronunciation_score'] = jstr.get("data").get("pronunciation_score")
    # rs['stress_score'] = jstr.get("data").get("stress_score")
    # rs['pause_score'] = jstr.get("data").get("pause_score")

    rs["pronunciation_score"] = pronunciation_score
    rs["basic_pause_score"] = basic_pause_score
    rs["basic_stress_score"] = basic_stress_score
    rs["project2_pause_score"] = project2_pause_score
    rs["project2_stress_score"] = project2_stress_score
    rs["project3_pause_score"] = project3_pause_score
    rs["project3_stress_score"] = project3_stress_score
    rs["project4_pause_score"] = project4_pause_score
    rs["project4_stress_score"] = project4_stress_score

    # return json.dumps(rs)
    return rs


def PracticalCompute():
    data = {
        "vowel_score": -1,
        "consonant_score": -1,
        "sentence_stress_score": -1,
        "words_stress_score": -1,
        "speech_speed_score": -1,
        "pause_score": -1
    }

    rs = {"error_no": 0,
          "error_msg": "succeed",
          "data": data
    }

    r = compute_common(1)
    print("adjust_score: %s chain_score %d" %(r,chain_score))
    jstr = adjust_score(r)
    logger.info("python result: %s " %jstr)

    rs['error_no'] = jstr.get("error_no")
    rs['error_msg'] = jstr.get("error_msg")
    data['vowel_score'] = jstr.get("data").get("vowel_score")
    data['consonant_score'] = jstr.get("data").get("consonant_score")
    data['sentence_stress_score'] = jstr.get("data").get("sentence_stress_score")
    data['words_stress_score'] = jstr.get("data").get("words_stress_score")
    data['speech_speed_score'] = jstr.get("data").get("speech_speed_score")
    data['pause_score'] = jstr.get("data").get("pause_score")

    return rs

def adjust_score(jstr):
    global chain_score
    jstr = json.loads(jstr)
    # 字典转json
    vowel_score = int(jstr.get("data").get("vowel_score"))
    consonant_score = int(jstr.get("data").get("consonant_score"))
    speech_speed_score = int(jstr.get("data").get("speech_speed_score"))

    # 计算停顿分数, 元音 辅音 语速 平均值上下浮动
    if vowel_score != -1 and consonant_score != -1 and chain_score > 20:
        pause_score = int(vowel_score + consonant_score + speech_speed_score) / 3 + int(random.randint(-5, 5))
        sentence_stress_score = int(vowel_score + consonant_score) / 2 + int(random.randint(-5, 5))
        words_stress_score = int(vowel_score) + int(random.randint(-5, 5))

        if pause_score >= 100:
            pause_score = pause_score - random.randint(5, 10)
        if sentence_stress_score >= 100:
            sentence_stress_score = sentence_stress_score - random.randint(5, 10)
        if words_stress_score >= 100:
            words_stress_score = words_stress_score - random.randint(5, 10)

        jstr["data"]["sentence_stress_score"] = sentence_stress_score
        jstr["data"]["words_stress_score"] = words_stress_score
        jstr["data"]["pause_score"] = pause_score

    return jstr

#local test
def ComputeTest():

    data = {
        "pronunciation_score": -1,
        "stress_score": -1,
        "pause_score": -1,
        "vowel_score": -1,
        "consonant_score": -1,
        "sentence_stress_score": -1,
        "words_stress_score": -1,
        "speech_speed_score": -1
    }

    rs = {"error_no": 0,
          "error_msg": "succeed",
          "data": data
          }

    fluency_info = {
        "SentenceStressScore": -1,
        "WordsStressScore": -1,
        "pauseScore": -1,
        "wordScore": -1,
    }

    global i
    start = datetime.datetime.now()
    print(start)
    print("******** time start %d*********" % (i % 11))

    para_info = request.get_json()
    normal_time = 0

    # request field wrong
    if ('wav-in-filename' not in para_info) or ('reference-text' not in para_info):
        return json.dumps(rs)
    # time field
    if "wav_time" in para_info and para_info['wav_time'] > 0:
        normal_time = str(para_info['wav_time'])

    wav_in_filename =str(para_info['wav-in-filename'])
    reference_text =str(para_info['reference-text'])

    wav_text = re.sub('[.,!?:]', '', reference_text)
    temp_text = re.sub('\[.*?\]', '', wav_text)
    list_text = temp_text.split(' ')
    # if no wav_time
    if normal_time == 0:
        normal_time = len(list_text) / normal_speed

    index = wav_in_filename.rfind('/')
    dir = wav_in_filename[0:index+1]

    convert_wav_file_name = dir+"test.wav"
    final_name = dir + "test_out.wav"

    if os.path.exists(convert_wav_file_name):
        os.remove(convert_wav_file_name)

    p = subprocess.Popen(['./ffmpeg', '-i', wav_in_filename, '-ar', '16000', '-ac', '1', convert_wav_file_name])
    p.communicate()

    #p = subprocess.Popen(['./rnnoise', convert_wav_file_name])
    #p.communicate()

    cps.ComputeScore(convert_wav_file_name, wav_text,2)
    r = cps.GetAppJsonStrResult()
    print(r)

    strlist = r.split('|')
    gop_info = json.loads(strlist[0])
    if strlist[1] != "":
        fluency_info = json.loads(strlist[1])
    else:
        fluency_info['SentenceStressScore'] = int(gop_info.get("SentenceStressScore"))
        fluency_info['pauseScore'] = int(gop_info.get("pauseScore"))
        fluency_info['WordsStressScore'] = int(gop_info.get("WordsStressScore"))
        fluency_info['wordScore'] = int(gop_info.get("wordScore"))
        json.dumps(fluency_info)

    data["pronunciation_score"] = int(gop_info.get("sentence_score"))
    data["vowel_score"] = int(gop_info.get("vowel_score"))
    data["consonant_score"] = int(gop_info.get("consonant_score"))

    data["stress_score"] = int(fluency_info.get("SentenceStressScore"))
    data["pause_score"] = int(fluency_info.get("pauseScore"))

    data["sentence_stress_score"] = int(fluency_info.get("SentenceStressScore"))
    data["words_stress_score"] = int(fluency_info.get("WordsStressScore"))

    real_time = gop_info.get("wav_time")

    print("normal_time: %f " % (len(list_text) / normal_speed))
    print("real_time: %f " % real_time)
    if (real_time >= int(normal_time)*0.8) and (real_time < int(normal_time)*1.2):
        data["speech_speed_score"] = random.randint(90,100)
    elif(
            (real_time < int(normal_time) * 0.8 and int(real_time) >= int(normal_time) * 0.5) or
            (real_time >= int(normal_time) * 1.2 and int(real_time) < int(normal_time) * 1.5)
        ):
        data["speech_speed_score"] = random.randint(80,90)
    elif(
            (real_time < int(normal_time) * 0.5 and int(real_time) >= int(normal_time) * 0.3) or
            (real_time >= int(normal_time) * 1.5 and int(real_time) < int(normal_time) * 1.8)
        ):
        data["speech_speed_score"] = random.randint(60, 80)
    elif (
            (real_time < int(normal_time) * 0.3 and int(real_time) >= int(normal_time) * 0.1) or
            (real_time >= int(normal_time) * 1.8 and int(real_time) < int(normal_time) * 2.0)
    ):
        data["speech_speed_score"] = random.randint(40, 60)
    else:
        data["speech_speed_score"] = random.randint(20, 40)

    # chain decode score
    word_score = int(fluency_info.get("wordScore"))
    if word_score > gop_info.get("sentence_score"):
        if word_score > 95:
            word_score = 95
        data["pronunciation_score"] = word_score
        data["vowel_score"] = word_score + 5
        data["consonant_score"] = word_score

        # 处理极端分数
    if word_score <= 5 and (word_score >= gop_info.get("sentence_score")):
        data["stress_score"] = -1
        data["sentence_stress_score"] = -1
        data["pause_score"] = -1
        data["pronunciation_score"] = -1
        data["vowel_score"] = -1
        data["consonant_score"] = -1
        data["speech_speed_score"] = -1
        data["words_stress_score"] = -1

        rs['error_msg'] = "please say again!"
        rs['error_no'] = -1

            # 考虑没有重音标记且识别正确的情况
    if data["pause_score"] == -1 and data["pronunciation_score"] > 0 and word_score > 5:
        data["pause_score"] = (data["pronunciation_score"] + data["speech_speed_score"]) / 2

        # 处理正常的地分数值
    if (word_score > 5) and (word_score <= 20) and (word_score > gop_info.get("sentence_score")):
        data["stress_score"] = word_score + random.randint(10, 15)
        data["sentence_stress_score"] = word_score + random.randint(10, 15)
        data["pause_score"] = word_score + random.randint(10, 15)
        data["pronunciation_score"] = word_score + random.randint(10, 15)
        data["vowel_score"] = word_score + random.randint(10, 15)
        data["consonant_score"] = word_score + random.randint(10, 15)
        data["words_stress_score"] = word_score + random.randint(10, 15)

        # 均一化重音分数
    if (word_score < 30) and (data["vowel_score"] < 30) and (data["consonant_score"] < 30) and (word_score > 0) and \
        data["vowel_score"] > 0:
        data["sentence_stress_score"] = data["vowel_score"] + random.randint(5, 10)
        data["words_stress_score"] = data["vowel_score"] + random.randint(5, 10)

    r = adjust_score(json.dumps(rs))
    print('r load %s' %r)

    if os.path.exists(convert_wav_file_name):
        os.remove(convert_wav_file_name)

    end = datetime.datetime.now()
    print(end)
    print("******** time end  %d*********" % (i % 11))
    i = i + 1
    if i == 11: i = 1

    return r

i=1

print('---------------------')
cps = libcompute.ComputeCenter()
normal_speed = 150/60
chain_score = 0
logging.basicConfig(level=logging.INFO)
app = connexion.App(__name__)
app.add_api('swagger.yaml')

application = app.app

if __name__ == "__main__":
    # app.run(port=8800, server='gevent', host='0.0.0.0', debug=True)
    app.run(port=8990)
