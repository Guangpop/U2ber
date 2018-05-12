#!/usr/bin/python

import argparse
import os
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError
from oauth2client.file import Storage
from oauth2client import client
from oauth2client import tools
from pprint import pprint

CLIENT_SECRETS_FILE = 'client_secret.json'

SCOPES = ['https://www.googleapis.com/auth/youtube.readonly']
API_SERVICE_NAME = 'youtube'
API_VERSION = 'v3'
APPLICATION_NAME = 'U2ber'

VALID_BROADCAST_STATUSES = ('all', 'active', 'completed', 'upcoming',)


# Authorize the request and store authorization credentials.
def get_authenticated_service(flags):
    credentials = get_credentials(flags)
    return build(API_SERVICE_NAME, API_VERSION, credentials=credentials)

liveChatId = ''

# Retrieve a list of broadcasts with the specified status.
def list_broadcasts(youtube_client, broadcast_status):
    print('Broadcasts with status "%s":' % broadcast_status)

    list_broadcasts_request = youtube_client.liveBroadcasts().list(
        broadcastStatus=broadcast_status,
        part='id,snippet',
        maxResults=50
    )

    while list_broadcasts_request:
        list_broadcasts_response = list_broadcasts_request.execute()

        for broadcast in list_broadcasts_response.get('items', []):
            print('%s (%s)(liveChatId: %s)' % (broadcast['snippet']['title'], broadcast['id'], broadcast['snippet']['liveChatId']))
            global liveChatId
            liveChatId = broadcast['snippet']['liveChatId']

        list_broadcasts_request = youtube.liveBroadcasts().list_next(
            list_broadcasts_request, list_broadcasts_response)


def list_streams(youtube):
    print("Live streams:")

    list_streams_request = youtube.liveStreams().list(
        part="id,snippet",
        mine=True,
        maxResults=50
    )

    while list_streams_request:
        list_streams_response = list_streams_request.execute()
        pprint(list_streams_response)
        for stream in list_streams_response.get("items", []):
            print("%s (%s)" % (stream["snippet"]["title"], stream["id"]))

        list_streams_request = youtube.liveStreams().list_next(
            list_streams_request, list_streams_response)


def get_live_chats(youtube_client):
    request = youtube_client.liveChatMessages().list(liveChatId=liveChatId, part='id,snippet,authorDetails')
    response = request.execute()
    pprint(response)



def get_credentials(args):
    home_dir = os.path.expanduser('~')
    credential_dir = os.path.join(home_dir, '.credentials')
    if not os.path.exists(credential_dir):
        os.makedirs(credential_dir)
    credential_path = os.path.join(credential_dir, 'u2ber.json')

    store = Storage(credential_path)
    credentials = store.get()
    if not credentials or credentials.invalid:
        flow = client.flow_from_clientsecrets(CLIENT_SECRETS_FILE, SCOPES)
        flow.user_agent = APPLICATION_NAME
        credentials = tools.run_flow(flow, store, args)
        print('Storing credentials to ' + credential_path)

    return credentials


if __name__ == '__main__':
    parser = argparse.ArgumentParser(parents=[tools.argparser])
    parser.add_argument('--broadcast-status', help='Broadcast status',
                        choices=VALID_BROADCAST_STATUSES, default=VALID_BROADCAST_STATUSES[0])

    args = parser.parse_args()

    youtube = get_authenticated_service(args)
    try:
        list_broadcasts(youtube, 'active')
        # get_live_chats(youtube)
        # list_broadcasts(youtube, 'all')
        list_streams(youtube)
    except HttpError as e:
        print('An HTTP error %d occurred:\n%s' % (e.resp.status, e.content))
