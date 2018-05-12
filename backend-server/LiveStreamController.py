from pprint import pprint
from googleapiclient.discovery import build

SCOPES = ['https://www.googleapis.com/auth/youtube.readonly']
API_SERVICE_NAME = 'youtube'
API_VERSION = 'v3'
APPLICATION_NAME = 'U2ber'

class LiveStreamController:

    def __init__(self, credentials):
        self.youtube = build(API_SERVICE_NAME, API_VERSION, credentials=credentials)

    def seek_livechatid(self):
        # Find live broadcast first, then find live stream(NYI, only live broadcast)
        # if don't, return None
        list_broadcasts_request = self.youtube.liveBroadcasts().list(
            broadcastStatus='active',
            part='id,snippet',
            maxResults=50
        )
        response = list_broadcasts_request.execute()
        livechat_id = None
        for broadcast in response.get('items', []):
            try:
                livechat_id = broadcast['snippet']['liveChatId']
                break
            except:
                pass

        return livechat_id

    def get_livechat(self, livechat_id):
        request = self.youtube.liveChatMessages().list(liveChatId=livechat_id, part='id,snippet,authorDetails')
        # while request:
        response = request.execute()
        for livechat in response.get('items', []):
            pprint(livechat)
            # request = self.youtube.liveChatMessages().list_next(request, request)

    def list_broadcasts(self, broadcast_status):
        print('Broadcasts with status "%s":' % broadcast_status)

        list_broadcasts_request = self.youtube.liveBroadcasts().list(
            broadcastStatus=broadcast_status,
            part='id,snippet',
            maxResults=50
        )

        while list_broadcasts_request:
            list_broadcasts_response = list_broadcasts_request.execute()
            for broadcast in list_broadcasts_response.get('items', []):
                try:
                    liveChatId = broadcast['snippet']['liveChatId']
                except:
                    liveChatId = ''
                print('%s (%s)(liveChatId: %s)' % (broadcast['snippet']['title'], broadcast['id'], liveChatId))

            list_broadcasts_request = self.youtube.liveBroadcasts().list_next(
                list_broadcasts_request, list_broadcasts_response)
