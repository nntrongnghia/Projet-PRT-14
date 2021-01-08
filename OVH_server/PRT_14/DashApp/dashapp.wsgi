#!/usr/bin/python3
import sys
import logging
logging.basicConfig(stream=sys.stderr)
sys.path.insert(0,"/home/ubuntu/PRT_14/DashApp/")
#sys.path.append('/usr/bin/python/')
from DashApp import server as application
#application.secret_key = 'Add your secret key'
