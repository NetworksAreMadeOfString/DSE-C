# ________          __           _________.__  _____  __   
# \______ \ _____ _/  |______   /   _____/|__|/ ____\/  |_ 
#  |    |  \\__  \\   __\__  \  \_____  \ |  \   __\\   __\
#  |    `   \/ __ \|  |  / __ \_/        \|  ||  |   |  |  
# /_______  (____  /__| (____  /_______  /|__||__|   |__|  
#         \/     \/          \/        \/                  
# ___________      ___.              .___  .___         .___
# \_   _____/ _____\_ |__   ____   __| _/__| _/____   __| _/
#  |    __)_ /     \| __ \_/ __ \ / __ |/ __ |/ __ \ / __ | 
#  |        \  Y Y  \ \_\ \  ___// /_/ / /_/ \  ___// /_/ | 
# /_______  /__|_|  /___  /\___  >____ \____ |\___  >____ | 
#         \/      \/    \/     \/     \/    \/    \/     \/ 
#
# Makefile 'o simpleness

withgit:
	git commit -a -m "pre-compile commit(`date`)"; g++ -mcpu=arm9 dse.c sbus.c -o dse -lcurl

all:
	g++ -mcpu=arm9 dse.c sbus.c -o dse -lcurl

verbose:
	g++ -mcpu=arm9 -Wall dse.c sbus.c -o dse -ldl -pthread -lcurl
