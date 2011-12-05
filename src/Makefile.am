
bin_PROGRAMS = balancc balanccd

balancc_SOURCES = balancc.cpp SocketHandler.cpp SocketServer.cpp SocketClient.cpp BalanccClient.cpp \
                              SocketHandler.h   SocketServer.h   SocketClient.h   BalanccClient.h
balancc_LDFLAGS = -lpthread -lrt

balanccd_SOURCES = balanccd.cpp SocketHandler.cpp BalanccServer.cpp Host.cpp \
                                SocketHandler.h   BalanccServer.h   Host.h   Slot.h
balanccd_LDFLAGS = -lpthread
