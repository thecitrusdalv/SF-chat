//client.cpp
#include "../headers/client.h"

using std::string;

Client::Client() = default;

Client::~Client()
{
	//Отправка уведомления об отключении
	disconnect();
}

void Client::connect(Server *server)
{
	currentServer = server;

	string choise; //Переменная для выбора пользователем

	for (;;) {
		std::cout << "login|register|exit: ";
		std::cin >> choise;
		std::cin.ignore(1000, '\n');

		if (choise == "login" || choise == "log") {
			if (authentication()) {
				userSpace();
			}
			continue;
		}

		if (choise == "register" || choise == "reg") {
			registration();
			continue;
		}

		if (choise == "exit")
			break;

		if (std::cin.fail()) {
			throw "cin fail. Client::connect()";
		}
	}
}

bool Client::registration() const
{
	std::cout << "\tType \"back\" to cancel" << std::endl;

	for (;;) {
		string login;
		//Ввод логина
		std::cout << "Login: ";
		std::cin >> login;
		std::cin.ignore(1000, '\n');

		if (login == "back")
			return false;

		if (std::cin.fail())
			throw "cin fail. Client::registrationt()";

		string pass;
		//Ввод пароля
		std::cout << "Password: ";
		std::cin >> pass;
		std::cin.ignore(1000, '\n');

		if (pass == "back")
			return false;

		if (std::cin.fail())
			throw "cin fail. Client::registrationt()";

		string nickname;
		//Ввод nickname
		std::cout << "Nickname: ";
		std::cin >> nickname;
		std::cin.ignore(1000, '\n');

		if (nickname == "back")
			return false;

		if (std::cin.fail())
			throw "cin fail. Client::registrationt()";

		//Создание запроса на основе введенных данных
		requestStructures::RegRequest req{std::move(login), std::move(pass), std::move(nickname)};

		//Отправка на сервер через метод request у сервера
		if (!currentServer->request(req)) {
			std::cerr << '\t' << req.result << std::endl;
			continue;
		} else {
			std::cout << '\t' << req.result << std::endl;
			break;
		}
	}
	return true;
}

bool Client::authentication()
{
	std::cout << "\tType \"back\" to cancel" << std::endl;

	for (;;) {
		string login, pass;
		std::cout << "Login: ";
		std::cin >> login; std::cin.ignore(1000, '\n');
		if (login == "back")
			return false;

		std::cout << "Password: ";
		std::cin >> pass; std::cin.ignore(1000, '\n');
		if (pass == "back")
			return false;

		if (std::cin.fail())
			throw ("cin fail. Client::logging() input");

		//Создание запроса на основе введенных данных,
		//Отправка запроса на сервер
		requestStructures::AuthRequest req{std::move(login), std::move(pass)};

		if (!currentServer->request(req)) {
			std::cerr << '\t' << req.result << std::endl;
			continue;
		} else {
			descriptor = req.clientDescr;
			return true;
		}
	}
}

/*
 * Пространство пользователя
 * Работа функции основана на главном бесконечном цикле
 * В каждой итерации пользователь приглашается к вводу
 * На основе введеной строки, определяется команда это
 * или нет.
 * Строка определяется как команда при наличии в начале
 * введенной строки символа :
 * Далее строка разделяется на саму команду и аргументы
 * к ней.
 * В case-ах оператора выбора switch описаны соответсующие
 * обработки команд.
 *
 * Если введенная строка - не команда, просто отправляется
 * запрос на отправку общего сообщения
*/

void Client::userSpace()
{
	//Запрос собственных сведений
	requestStructures::MySelfRequest mySelf{descriptor};
	if (!currentServer->request(mySelf)) {
		std::cerr << '\t' << mySelf.result << std::endl;
		return;
	}

	//Показ общих сообщений на сервере
	if (!showGlobalMessages())
		return;

	string input; //Строка для ввода

	std::vector<string> commands = {
		"list", "user", "priv", "help"
	};
	enum enumCommands {
		LIST, USER, PRIV, HELP
	};

	//Главный цикл userSpace()
	for (;;) {
		
		//Вывод никнейма и запрос на ввод
		std::cout << "->" << mySelf.nickname << ": ";
		std::getline(std::cin, input);
		if (std::cin.fail())
			throw ("cin fail. Client::userSpace()");

		if (input[0] == ':') {

			string temp; //Заводится временная строка.
			std::vector<string> CommandArgsVec; //Заводится вектор с командой и ее аргументами.

			//Данный цикл разделяет ввод на отдельные слова, добавляя их в вектор
			//CommandArgsVec
			for (size_t i = 1; i < input.size(); i++) {
				if (input[i] == ' ') {
					if (!temp.empty()) {
						CommandArgsVec.push_back(temp);
						temp.clear();
					}
				} else {
					temp += input[i];
				}
			}
			if (!temp.empty()) {
				CommandArgsVec.push_back(temp);
				temp.clear();
			}

			//Команда :out или :q осуществляет выход из учетной записи, путем прерывания главного цикла
			//пространства пользователя
			if ((CommandArgsVec[0] == "out") || (CommandArgsVec[0] == "q")) {
				break;
			}

			if (CommandArgsVec[0] == "exit")
				exit(0);

			int command_number = -1;
			for (size_t i = 0; i < commands.size(); i++) {
				if (CommandArgsVec[0] == commands[i]) {
					command_number = i;
				}
			}

			//Определение команды
			switch (command_number) {
				case LIST: //Вывод пользователей
					currentServer->show();
					break;

				//Определяет дальнешие действия с определенным юзером на сервере
				case USER: 
					if (CommandArgsVec.size() < 2) {
						std::cout << "\tCommand <user> must have at least one argument" << std::endl;
						break;
					}

					//Если аргумент команды один, он используется как идентификатор пользователя
					if (CommandArgsVec.size() == 2) {
						string message;
						std::cout << "\tMessage to " << CommandArgsVec[1] << ": ";
						std::getline(std::cin, message);

						requestStructures::UserMessageRequest req{descriptor, CommandArgsVec[1], std::move(message)};

						if(!currentServer->request(req))
							std::cerr << '\t' << req.result << std::endl;
					}

					//Если аргументов два и более, начиная со второго, они идентифицируются как
					//сообщение пользователю
					if (CommandArgsVec.size() > 2) {
						string message;
						for (size_t i = 2; i < CommandArgsVec.size(); i++) {
							message += CommandArgsVec[i];
							if (i != CommandArgsVec.size()-1)
								message += ' ';
						}

						requestStructures::UserMessageRequest req{descriptor, CommandArgsVec[1], std::move(message)};

						if(!currentServer->request(req))
								std::cerr << '\t' << req.result << std::endl;
					}

					break;

				case PRIV: //Показ приватных сообщений
					showPersonalMessages();
					break;

				case HELP: //Вывод справки
					std::cout << "\tPossible commands:\n" <<
					"\t:list\t - listing of all users in current room;\n" <<
					"\t:user\t - send private message to other users " <<
						"(:user <name> or :user <name> <message>)\n" <<
					"\t:priv\t - show private messages from other users\n\n" <<
					"\t:out/q\t - logout from current user\n" <<
					"\t:exit\t - exit from program" << std::endl;
					break;

				default:
					std::cout << "\tCommand not found" << std::endl;
					break;
			}

		} else {

			//Если небыло команд, добавление общего сообщения на сервер
			requestStructures::GlobalMessageRequest req{descriptor, std::move(input)};

			if(!currentServer->request(req)) {
				std::cerr << '\t' << req.result << std::endl;
				std::cerr << "Cannot send message to server" << std::endl;
			}

			if (!showGlobalMessages())
				return;
		}
	}
}

//Обертка запроса на вывод личных сообщений
bool Client::showPersonalMessages() const
{
	requestStructures::PersonalMessagesRequest req{descriptor};

	if (!currentServer->request(req)) {
		std::cerr << '\t' << req.result << std::endl;
		std::cerr << "\tCannot load messages from server" << std::endl;
		return false;
	}
	else
		return true;
}

//Обертка запроса на вывод общих сообщений
bool Client::showGlobalMessages() const
{
	requestStructures::AllMessagesRequest req{descriptor, DEFAULT_SHOW_MESSAGES_COUNT};
	if (!currentServer->request(req)) {
		std::cerr << '\t' << req.result << std::endl;
		std::cerr << "Cannot load messages from server" << std::endl;
		return false;
	} else
		return true;
}

//Обертка уведомления об отключении
//Очистка выданного дескриптора
//обнуление указателя на сервер
void Client::disconnect()
{
	if (!descriptor.empty()) {
		requestStructures::DisconnectRequest req{descriptor};

		if (!currentServer->request(req)) {
			std::cerr << '\t' << req.result << std::endl;
		}

		descriptor.clear();
	}
		currentServer = nullptr;
}
