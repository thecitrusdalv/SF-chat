//client.h
#pragma once

#include <iostream>
#include <vector>

#include "server.h"

#define DEFAULT_SHOW_MESSAGES_COUNT 30 //кол-во глобальных сообщений, которое клиент просит показать сервер

class Client
{
public:
    Client();
    ~Client();

	void connect(Server*); //Подключение к серверу
	void disconnect(); //Отключение

private:
	std::string descriptor; //Дескриптор выданный сервером
	Server *currentServer = nullptr; //Текущий сервер

	bool registration() const; //Обертка запроса для регистрации
	bool authentication(); //Обертка запроса аутентификации
	bool showPersonalMessages() const; //Обертка запроса личных сообщений
	bool showGlobalMessages() const; //Обертка запроса общих сообщений
	void userSpace(); //Пространство пользователя
};
