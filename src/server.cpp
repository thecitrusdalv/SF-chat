//server.cpp
#include "../headers/server.h"

using std::string;

Server::Server()
{
	//Инициализация фаловой подсистемы. Если неудачно, выброс исключения
	if (fileSubsystem.init()) {

		loadUsersFromFile(fileSubsystem.usersFile); //Загрузка пользователей из файла в память

		//Загрузка глобальных сообщений из файла в память.
		loadGlobalMessagesFromFile(fileSubsystem.globalMessagesFile, MESSAGES_LOAD_COUNT);

	} else
		throw "Can't initialize file subsystem";
}

Server::~Server()
{
	//Освобождение памяти от пользователей
	for (auto x : logins) {
		delete x.second;
	}
}

Server::Server(const Server&)		= delete;
Server::Server(const Server&&)	= delete;

Server& Server::operator= (const Server&)		= delete;
Server& Server::operator= (const Server&&)	= delete;

/*
 * Функции обработки запросов.
 * В каждом запросе, завернутом в виде структры, есть поле для
 * результата работы обработки. Клиент, может использовать
 * это поле для идентификации результата обработки запроса сервером.
 * Сервер кладет информацию в структуру запроса, в поле result
 *
 * В каждом запросе, за исключением аутентификации, должен быть
 * дескриптор, который выдается сервером после аутентификации
 *
*/

//Обработка запроса аутентификации
bool Server::request(requestStructures::AuthRequest& req)
{
	//Если логин не существует
	if (!loginExist(req.login)) {
		req.result = "Login not found";
		return false;
	}

	//Находим в карте итератор, соответствующий логину в запросе,
	//Вытаскиваем пароль и сравниваем с тем, что в запросе.
	//В случае успеха, генерируется дескриптор и кладется в запрос.
	//Клиент использует дескриптор в дальнейших запросах для
	//идентификации
	
	auto it = logins.find(req.login);

	if (req.pass == it->second->pass) {

		string newDescriptor;
		for (int i = 0; i < 20; i++) {
			newDescriptor += char(rand() % 26 + 'a');
		}

		issuedDescriptors[newDescriptor] = req.login;
		req.clientDescr = std::move(newDescriptor);
		req.result = "Success";
		return true;

	} else {
		req.result = "Incorrect password";
		return false;
	}
}

//Обработка запроса на регистрацию нового пользователя
//Проверка внутри на доступность и валидность идентификаторов,
//отправленных в запросе клиентом
bool Server::request(requestStructures::RegRequest& req)
{
	if (!name_check(req.login, req.result))
		return false;

	if (!name_check(req.nickname, req.result))
		return false;

	if (loginExist(req.login)) {
		req.result = "Login unavailable";
		return false;
	}

	if (nicknameExist(req.nickname)) {
		req.result = "Nickname unavailable";
		return false;
	}

	addUser(std::move(req.login), std::move(req.pass), std::move(req.nickname));
	req.result = "Success";

	return true;
}

//Запрос на отправку сообщения на сервер
bool Server::request(requestStructures::GlobalMessageRequest& req)
{
	if (!descriptorExist(req.clientDescr)) {
		req.result = "Account unavailable";
		return false;
	}

	auto& nickname = logins.find(issuedDescriptors[req.clientDescr])->second->nickname;

	messages.emplace_front(nickname, req.message);
	fileSubsystem.globalMessagesFile << nickname << ' ' << req.message << std::endl;

	req.result = "Message recieved";
	return true;
}

//Обраотка запроса на отправку приватного сообщения пользователю
bool Server::request(requestStructures::UserMessageRequest& req)
{
	if (!descriptorExist(req.clientDescr)) {
		req.result = "Account unavailable";
		return false;
	}

	if (!nicknameExist(req.nickname)) {
		req.result = "User not found";
		return false;
	}

	const Server::User& sender = *(logins.find(issuedDescriptors[req.clientDescr])->second);
	Server::User& reciever = *(logins.find(req.nickname)->second);

	//Кладем адресату персональное сообщение в его файл
	std::fstream recieverFile;
	if (!fileSubsystem.bindUserPersonalFile(reciever.login, recieverFile))
		return false;

	recieverFile.seekg(0, std::ios_base::end);
	recieverFile << sender.nickname << ' ' << std::move(req.message) << std::endl;
	recieverFile.close();

	req.result = "Message recieved";
	return true;
}

//Обработка запроса на показ личных сообщений
bool Server::request(requestStructures::PersonalMessagesRequest& req)
{
	if (!descriptorExist(req.clientDescr)) {
		req.result = "Account unavailable";
		return false;
	}

	const Server::User& user = *(logins.find(issuedDescriptors[req.clientDescr])->second);

	std::fstream userPersonalMessagesFile;

	if(!fileSubsystem.bindUserPersonalFile(user.login, userPersonalMessagesFile))
		return false;

	string input;
	while (userPersonalMessagesFile >> input) {
		std::cout << '\t' << input << ": ";

		getline(userPersonalMessagesFile, input);
		std::cout << input << std::endl;
	}

	userPersonalMessagesFile.close();
	
	req.result = "Messages transmitted";
	return true;
}

//Обработка запроса на показ общих сообщений
bool Server::request(requestStructures::AllMessagesRequest& req)
{
	if (!descriptorExist(req.clientDescr)) {
		req.result = "Account unavailable";
		return false;
	}

	auto current = messages.end()-1;
	if (messages.size() > req.count)
		current = messages.begin()+req.count-1;
#ifdef LINUX
	std::system("clear");
#endif
	std::cout << "-------------------------\n";
	while (current >= messages.begin()) {
		std::cout << '\t' << current->own << ": " << current->msg << '\n';
		current--;
	}

	if (messages.size() > MESSAGES_MAXIMUM_STORED) {
		messages.resize(MESSAGES_LOAD_COUNT);
	}

	std::cout << std::endl;
	req.result = "Messages transmitted";
	return true;
}

//Обработка запроса собственных актуальных сведений
bool Server::request(requestStructures::MySelfRequest& req)
{
	if (!descriptorExist(req.clientDescr)) {
		req.result = "Account unavailable";
		return false;
	}

	const Server::User& user = *(logins.find(issuedDescriptors[req.clientDescr])->second);

	req.login = user.login;
	req.nickname = user.nickname;

	req.result = "User info transmitted";
	return true;
}

//Обработка уведомления об отключении клиента
//Уничтожение выданного дескриптора
bool Server::request(requestStructures::DisconnectRequest& req)
{
	if (!descriptorExist(req.clientDescr)) {
		req.result = "Account unavailable";
		return false;
	}

	auto user_it = issuedDescriptors.find(req.clientDescr);
	issuedDescriptors.erase(user_it);

	req.result = "Disconnected";
	return true;
}

//Добавление нового пользователя
//Логин и псевдоним ассоциируется в своих map с определенным адресом созданного пользователя
void Server::addUser(const string& login, const string& pass, const string& nickname)
{
	loadUserToMemory(std::move(login), std::move(pass), std::move(nickname));
	fileSubsystem.usersFile << login << ' ' << pass << ' ' << nickname << std::endl;
}

//Удаление пользователя
//Уничтожаются записи в обоих map, память, выданная на пользователя, освобождается
bool Server::delUser(const string& login)
{
	auto loginIt = logins.find(login);
	if (loginIt == logins.end())
		return false;

	string nick = loginIt->second->nickname;
	auto nicknameIt = nicknames.find(nick);

	delete loginIt->second;

	logins.erase(loginIt);
	nicknames.erase(nicknameIt);

	return true;
}

//Проверка на валидность идентификатора
bool Server::name_check(const string& name, string& result) const
{
	if (name.empty() || name.size() > NAME_MAX_SIZE) {
		result = "Cannnot be empty and more then ";
		result += std::to_string(NAME_MAX_SIZE);

		return false;
	}

	for (char i : name) {
		if (i < '!' || i > '~') {
			result = "Must containe from ! to ~ characters in ASCII table";
			return false;
		}
	}

	return true;
}

//Существует ли логин?
bool Server::loginExist(const string& login) const
{
	return logins.find(login) == logins.end() ? false : true;
}

//Существует ли псевдоним?
bool Server::nicknameExist(const string& nickname) const
{
	return nicknames.find(nickname) == nicknames.end() ? false : true;
}

//Выдан ли дескриптор?
bool Server::descriptorExist(const string& descr) const
{
	return issuedDescriptors.find(descr) == issuedDescriptors.end() ? false : true;
}

//Вывод всех пользователей
void Server::show() const
{
	for (auto x : logins) {
		std::cout << '\t' << x.second->nickname << std::endl;
	}
}

//Добавление пользователя в память
void Server::loadUserToMemory(const string& login, const string& pass, const string& nickname)
{
	User *newUser		= new User(login, nickname, pass);
	logins[login]		= newUser;
	nicknames[nickname]	= newUser;
}

//Загрузка в память пользователей из файла
bool Server::loadUsersFromFile(std::fstream& file)
{
	while (file) {
		string login, pass, nickname;
		file >> login >> pass >> nickname;

		loadUserToMemory(std::move(login), std::move(pass), std::move(nickname));
	}

	file.clear(); //Сброс флага конца файла
	return true;
}

//Загрузка глобальных сообщений в память из файла
void Server::loadGlobalMessagesFromFile(std::fstream& file, const size_t need_lines)
{
	/*
	 * need_lines - параметр, определяющий какое нужно кол-во строк, считая
	 * с конца файла.
	 * Сначала идем в коней файла, определяем позицию. Если позиция нулевая,
	 * значит файл пустой, возвращаем управление.
	 * Заводим счетчик кол-ва обнаруженных строк.
	 * Пока рассматриваемая позиция не дошла до начала файла или не набралось
	 * количество необходимых строк, идем по файлу назад.
	 * Встаем на начало нужной строки. Считываем файл.
	 */

	file.seekg(0, std::ios_base::end);
	size_t cur_pos = file.tellg();

	if (cur_pos == 0)
		return;

	size_t lines_count = 0;

	while (cur_pos != 0) {
		int c = file.peek();

		if (c == '\n')
			lines_count++;

		if (lines_count == need_lines+1) {
			file.seekg(cur_pos+1);
			break;
		}

		file.seekg(--cur_pos);
	}

	std::string own, msg;

	while (file >> own) {
		getline(file, msg);
		messages.emplace_front(std::move(own), std::move(msg));
	}

	file.clear();
}
