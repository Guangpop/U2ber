from LiveStreamController import LiveStreamController
from APITokenManagement import OAuthTokenManager

credentials = OAuthTokenManager().get_credentials()
controller = LiveStreamController(credentials)
liveChatId = controller.seek_livechatid()
controller.get_livechat(liveChatId)