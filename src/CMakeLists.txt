cmake_minimum_required(VERSION 3.10)
project(cliente_t2_redes)

set(CMAKE_CXX_STANDARD 14)

set(Client Client.cpp Client.h)
set(Timer Timer.h)
set(InputProcessor InputProccesor.h InputProccesor.cpp)
set(Packet Packet.h)
set(DataProcessor DataProcessor.h DataProcessor.cpp)
set(ClientSender ClientSender.h ClientSender.cpp)
set(ClientReceiver ClientReceiver.h ClientReceiver.cpp)
set(ClientSubject ClientSubject.h)

set(INNER_SOURCE_FILES main.cpp
        ${InputProcessor}
        ${Client}
        ${Timer}
        ${Packet}
        ${DataProcessor}
        ${ClientSubject}
        ${ClientSender}
        ${ClientReceiver}
        )

set(RES_LIST "")
foreach (SF IN LISTS INNER_SOURCE_FILES)
    list(APPEND RES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/${SF}")
endforeach ()

set(SOURCEFILES ${RES_LIST} PARENT_SCOPE)
