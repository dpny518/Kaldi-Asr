import connexion
import logging
import flask
import fluency
import sys
import stat
import wave
import contextlib
import subprocess
import requests
import os.path
import time

from flask import request

reload(sys)
sys.setdefaultencoding('utf-8')

logger = logging.getLogger("fluency")
formatter = logging.Formatter('%(asctime)s %(levelname)-8s: %(message)s')

file_handler = logging.FileHandler("fluency.log")
file_handler.setFormatter(formatter)

logger.addHandler(file_handler)

# help functions
def download_file(url):

        start = time.time()
        logger.info("downloading "+url)

        r = requests.get(url)
        end = time.time();
        logger.info("downloading time: "+str(end-start))

        if os.path.isfile(os.path.basename(url)):
            os.remove(os.path.basename(url))

        if not r.status_code == 200:
            return False
        with open(os.path.basename(url), "wb") as code:
            code.write(r.content)

        return True


def wav_format_convert(download_wav_file):

    if os.path.exists(download_wav_file)==False:
        return False

    convert_wav_file_name = "test.wav"

    if os.path.isfile(convert_wav_file_name):
        os.remove(convert_wav_file_name)

    p = subprocess.Popen(['./ffmpeg', '-i',download_wav_file, '-ar', '16000', '-ac', '1',convert_wav_file_name])

    p.communicate()

  #  if returncode != 0:
   #     return False

    return True

def fluecnyScore():

    data = request.get_json()

    fScore = fluency.Score()

    wav_in_filename = str(data['wav_in_file_path'])
    compare_sentence = str(data['compare_sentence'])

    if download_file(wav_in_filename) == False:
        logger.info("download file failed!")
        return 0

    if wav_format_convert(os.path.basename(wav_in_filename)) == False:
        logger.info("convert file failed!")
        return 0

    wav_in_filename = 'test.wav'

    result = fScore.GetSpeechFluencyJson(wav_in_filename, compare_sentence, 1)

    return result

logging.basicConfig(level=logging.INFO)
app = connexion.App(__name__)
app.add_api('swagger.yaml')

application = app.app

if __name__ == "__main__":
    app.run(port=8990)