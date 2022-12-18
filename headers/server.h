//server.h
#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <deque>
#include <cstdlib>
#include <cstring>
#include <string_view>

#define NAME_MAX_SIZE 20 //Максимальное количество символов идентификаторов пользователя
#define MESSAGES_LOAD_COUNT 15 //Кол-во сообщений, подтягиваемых из файла
#define FILLING_RATIO 1.5 //Коэффициент наполнения очереди глобальных сообщений
#define USERS_FILE "./relations/users" //Файл для хранения пользователей
#define GLOBAL_MESSAGES_FILE "./relations/global_messages" //Файл хранения глобальных сообщений

//Директория для хранения файлов пользователей с личными сообщениями
#define PERSONAL_MESSAGES_PATH "./relations/personal/" 

/*
 * Определение максимального кол-ва сообщений, которое сервер будет хранить в памяти.
 * Зависит от кол-ва сообщений, которое сервер загружает в память из файла и коэффициента.
 * При достижении большего кол-ва, чем максимальное, сервер автоматически высвобождает
 * старые сообщения.
*/
constexpr size_t MESSAGES_MAXIMUM_STORED = MESSAGES_LOAD_COUNT * FILLING_RATIO;

using std::string;
namespace fs = std::filesystem;

//Структуры для запросов на сервер
namespace requestStructures
{
	//Запрос на аутентификацию
	struct AuthRequest
	{
		AuthRequest(const string& _login, const string& _pass) :
			login(_login), pass(_pass)
		{}

		const string login, pass;

		string result;
		string clientDescr;
	};

	//Запрос на регистрацию
	struct RegRequest
	{
		RegRequest(
			const string& _login,
			const string& _pass,
			const string& _nick
		) : login(_login), pass(_pass), nickname(_nick)
		{}

		const string login, pass, nickname;

		string result;
	};

	//Запрос на отправку общего сообщения
	struct GlobalMessageRequest
	{
		GlobalMessageRequest(const string& _descr, const string& _msg) :
			clientDescr(_descr), message(_msg)
		{}

		const string clientDescr;
		const string message;

		string result;
	};

	//Запрос на отправку приватного сообщения
	struct UserMessageRequest
	{
		UserMessageRequest(const string& _descr, const string& _nick, const string& _msg) :
			clientDescr(_descr), nickname(_nick), message(_msg)
		{}

		const string clientDescr;
		const string nickname;
		const string message;

		string result;
	};

	//Запрос на просмотр личных сообщений
	struct PersonalMessagesRequest
	{
		PersonalMessagesRequest(const string& _descr) :
			clientDescr(_descr)
		{}

		const string clientDescr;

		string result;
	};

	//Запрос на просмотр общих сообщений
	struct AllMessagesRequest
	{
		AllMessagesRequest(const string& _descr, size_t _count) :
			clientDescr(_descr), count(_count)
		{}

		const string clientDescr;
		const size_t count;

		string result;
	};

	//Запрос клиентом сведений о себе
	struct MySelfRequest
	{
		MySelfRequest(const string& _descr) :
			clientDescr(_descr)
		{}

		const string clientDescr;

		string login, nickname;
		string result;
	};

	//Уведомление об отключении
	struct DisconnectRequest
	{
		DisconnectRequest(const string& _descr) :
			clientDescr(_descr)
		{}

		const string clientDescr;

		string result;
	};
}

class Server
{
public:

	//Структура сообщений
	struct Message
	{
		Message();
		Message(const string&, const std::string&);
		Message(const Message&);
		Message(Message&&);
		Message& operator= (const Message&);
		Message& operator= (Message&&);
		~Message();

		string own, msg;
	};

	//Структура пользователей
	struct User
	{
		User(const string&, const std::string&, const std::string&);
		User(const User&);
		User(User&&);
		User& operator= (const User&);
		User& operator= (User&&);
		~User();

		string login, nickname, pass;
	};

	Server();
	~Server();
	Server(const Server&);
	Server(const Server&&);
	Server& operator= (const Server&);
	Server& operator= (const Server&&);

	//Обработчики запросов
	bool request(requestStructures::AuthRequest&);
	bool request(requestStructures::RegRequest&);
	bool request(requestStructures::GlobalMessageRequest&);
	bool request(requestStructures::UserMessageRequest&);
	bool request(requestStructures::PersonalMessagesRequest&);
	bool request(requestStructures::AllMessagesRequest&);
	bool request(requestStructures::MySelfRequest&);
	bool request(requestStructures::DisconnectRequest&);

	//Вывод всех пользователей
	void show() const;

private:
 /*
 * Файловая подсистема.
 * Осуществляет создание, открытие необходимых для работы файлов.
 * Выставляет необходимые права на файлы.
 * Предоставляет серверу готовые файловые потоки.
 */

	class FileSubsystem
	{
	public:
		FileSubsystem() = default;
		~FileSubsystem() = default;

		std::fstream globalMessagesFile; //Файловый поток глобальных сообщений
		std::fstream usersFile; //Файловый поток со сведениями зарегистрированных пользователей

		bool init(); //Инициализация подсистемы

		//Связывание переданного потока с нужным файлом пользователя
		bool bindUserPersonalFile (const string&, std::fstream&);

	private: 
		bool open_file(std::fstream&, const string&); //Открытие(создание) файла подсистемой
		void set_permissions(const string&); //Выставление прав на файл
		const char* diagnosticMessageHead(); //Заголовок диагностического сообщения. При ошибках
	}
	fileSubsystem; //По дефолту сразу определен экземпляр класса

	std::map<string, User*> logins; //Карта логинов
	std::map<string, User*> nicknames; //Карта псевдонимов
	std::map<string, std::string> issuedDescriptors; //Карта выданных дескрипторов

	std::deque<Message> messages; //Очередь общих сообщений

	void addUser(const string&, const std::string&, const std::string&); //Добавление нового пользователя
	void loadUserToMemory(const string&, const std::string&, const std::string&);
	bool loadUsersFromFile(std::fstream&);
	void loadGlobalMessagesFromFile(std::fstream&, const size_t);
	bool delUser(const string&); //Удаление
	bool name_check(const string&, std::string&) const; //Валидация идентификаторов пользователей
	bool loginExist(const string&) const; //Проверка существования логина
	bool nicknameExist(const string&) const; //Проверка существования псевдонима
	bool descriptorExist(const string&) const; //Проверка существования дескриптора
};
