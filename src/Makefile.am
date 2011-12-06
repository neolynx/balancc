
bin_PROGRAMS = balancc
balancc_SOURCES = balancc.cpp SocketHandler.cpp SocketServer.cpp SocketClient.cpp BalanccClient.cpp \
                              SocketHandler.h   SocketServer.h   SocketClient.h   BalanccClient.h
balancc_LDFLAGS = -lpthread -lrt

sbin_PROGRAMS = balanccd
balanccd_SOURCES = balanccd.cpp SocketHandler.cpp BalanccServer.cpp Host.cpp \
                                SocketHandler.h   BalanccServer.h   Host.h   Slot.h
balanccd_LDFLAGS = -lpthread
