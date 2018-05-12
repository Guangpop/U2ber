import os
import argparse
from oauth2client.file import Storage
from oauth2client import client
from oauth2client import tools

CLIENT_SECRETS_FILE = 'client_secret.json'
SCOPES = ['https://www.googleapis.com/auth/youtube.readonly']
APPLICATION_NAME = 'U2ber'


class OAuthTokenManager:
    def __init__(self):
        self.parser = argparse.ArgumentParser(parents=[tools.argparser])
        home_dir = os.path.expanduser('~')
        credential_dir = os.path.join(home_dir, '.credentials')
        if not os.path.exists(credential_dir):
            os.makedirs(credential_dir)
        self.credential_path = os.path.join(credential_dir, 'u2ber.json')

    def __do_web_auth(self):
        flow = client.flow_from_clientsecrets(CLIENT_SECRETS_FILE, SCOPES)
        flow.user_agent = APPLICATION_NAME
        credentials = tools.run_flow(flow, Storage(self.credential_path), self.parser.parse_args())
        print('Storing credentials to ' + self.credential_path)
        return credentials

    def __do_file_auth(self):
        return Storage(self.credential_path).get()

    def get_credentials(self):
        credentials = self.__do_file_auth()
        if not credentials or credentials.invalid:
            return self.__do_web_auth()

        return credentials

    def get_arg_parser(self):
        return self.parser

