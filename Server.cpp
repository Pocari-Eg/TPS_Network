#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

#define _WIN32_WINNT 0x0601


using namespace boost;
using std::cout;
using std::endl;
using std::string;


enum State {
	IDLE =0,
	WALK,
};

struct Replication
{
	float PosX, PosY, PosZ=73.0f;
	float RotZ;
	State state;
};



struct Session
{
	shared_ptr<asio::ip::tcp::socket> sock;
	asio::ip::tcp::endpoint ep;
	string id;
	int room_no = -1;
	string sbuf;
	string rbuf;
	Replication repli;
	char buf[80];

};
std::vector<string> PlayerList;

bool CompareSessions(const Session& a, const Session& b) {
	std::cout << "Comparing IDs: " << a.id << " and " << b.id << std::endl;
	return a.id < b.id;
}

class Server
{
	asio::io_service ios;
	shared_ptr<asio::io_service::work> work;
	asio::ip::tcp::endpoint ep;
	asio::ip::tcp::acceptor gate;
	std::vector<Session*> sessions;
	boost::thread_group threadGroup;

	boost::mutex lock;
	std::vector<int> existingRooms;
	const int THREAD_SIZE = 5;

	boost::asio::steady_timer replicationTimer;

	enum Code { INVALID, SET_ID,REP,HIT };

public:
	Server(string ip_address, unsigned short port_num) :
		work(new asio::io_service::work(ios)),
		ep(asio::ip::address::from_string(ip_address), port_num),
		gate(ios, ep.protocol()),replicationTimer(ios)
	{
	
		existingRooms.push_back(0);
	}

	

	void Start()
	{

		cout << "Start Server" << endl;
		cout << "Creating Threads" << endl;
		for (int i = 0; i < THREAD_SIZE; i++)
			threadGroup.create_thread(bind(&Server::WorkerThread, this));

		// thread �� ������������� ��� ��ٸ��� �κ�
		this_thread::sleep_for(chrono::milliseconds(100));
		cout << "Threads Created" << endl;

		boost::asio::socket_base::reuse_address option(true);
		gate.set_option(option);
		ios.post(bind(&Server::OpenGate, this));
		replicationTimer.expires_from_now(std::chrono::milliseconds(100)); // �ʱ� Ÿ�̸� ���� �ð� ����
		replicationTimer.async_wait(boost::bind(&Server::SendReplication, this)); // Ÿ�̸� ����
		threadGroup.join_all();
	}

private:
	void WorkerThread()
	{
		lock.lock();
		cout << "[" << boost::this_thread::get_id() << "]" << " Thread Start" << endl;
		lock.unlock();

		ios.run();

		lock.lock();
		cout << "[" << boost::this_thread::get_id() << "]" << " Thread End" << endl;
		lock.unlock();
	}

	void OpenGate()
	{
		system::error_code ec;
		gate.bind(ep, ec);
		if (ec)
		{
			cout << "bind failed: " << ec.message() << endl;
			return;
		}

		gate.listen();
		cout << "Gate Opened" << endl;

		StartAccept();
		cout << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
	}

	// �񵿱�� Accept
	void StartAccept()
	{
		Session* session = new Session();
		shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(ios));
		session->sock = sock;
		gate.async_accept(*sock, session->ep, bind(&Server::OnAccept, this, _1, session));
	}

	void OnAccept(const system::error_code& ec, Session* session)
	{
		if (ec)
		{
			cout << "accept failed: " << ec.message() << endl;
			return;
		}

		lock.lock();
		sessions.push_back(session);
		cout << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;
		lock.unlock();

		ios.post(bind(&Server::Receive, this, session));

		StartAccept();
	}

	// ����� Receive (�����尡 ������ ������ 1:1 ���)
	void Receive(Session* session)
	{
		system::error_code ec;
		size_t size;
		size = session->sock->read_some(asio::buffer(session->buf, sizeof(session->buf)), ec);

		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
			CloseSession(session);
			return;
		}

		if (size == 0)
		{
			cout << "[" << boost::this_thread::get_id() << "] peer wants to end " << endl;
			CloseSession(session);
			return;
		}

	

		session->buf[size] = '\0';
		session->rbuf = session->buf;
		PacketManager(session);
		//cout << "[" << boost::this_thread::get_id() << "] " << session->rbuf << endl;

		Receive(session);
	}



	void PacketManager(Session* session)
	{
		// :~ ��� Ư��(?)�޼����� �������� ��� ó��
		if (session->buf[0] == ':')
		{
			Code code = TranslatePacket(session->rbuf);

			switch (code)
			{
			case Code::SET_ID:
				SetID(session);
				break;
			case Code::REP:
				ReplicationUpdate(session);
				break;
			case Code::HIT:
				PlayerHit(session);
				break;
			case Code::INVALID:
				session->sbuf = "��ȿ���� ���� ��ɾ� �Դϴ�";
				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
				break;
			}
		}
		else  // :~ ��� Ư���޼����� �ƴϰ� �׳� ä���� ���
		{
			if (session->id.length() != 0) // id length�� 0�� ���� id�� ���� ������� ���� ���
			{
				string temp = "[" + session->id + "]:" + session->rbuf;
				SendAll(session, session->room_no, temp, false);
			}
			else
			{
				session->sbuf = "Register your ID first through :set";
				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
			}
		}
	}

	Code TranslatePacket(string message)
	{
		string temp = message.substr(0, sizeof(":set ") - 1);
		// :set �� ���
		if (temp.compare(":set ") == 0)
		{
			return Code::SET_ID;
		}
		temp = message.substr(0, sizeof(":rep ") - 1);
		if (temp.compare(":rep ") == 0)
		{
			return Code::REP;
		}
		temp = message.substr(0, sizeof(":hit ") - 1);
		if (temp.compare(":hit ") == 0)
		{
			return Code::HIT;
		}
		return Code::INVALID;
	}
	void ReplicationUpdate(Session* session)
	{
		string temp = session->rbuf.substr(sizeof(":rep ") - 1, session->rbuf.length());

		Replication data = deserializeReplication(temp);
		
		session->repli.PosX = data.PosX;
		session->repli.PosY = data.PosY;
		session->repli.PosZ = data.PosZ;
		session->repli.RotZ = data.RotZ;
		session->repli.state = data.state;

	}
	void PlayerHit(Session* session)
	{
		string temp = session->rbuf.substr(sizeof(":hit ") - 1, session->rbuf.length());
		
		std::istringstream ss(temp);
		std::string token;

		int index;
		int damage;
		// ��ǥ�� ���е� �� �κ��� �����ϰ� ������ ��ȯ
		if (std::getline(ss, token, ','))
		{
			index = std::stoi(token);
		}

		if (std::getline(ss, token, ','))
		{
			damage = std::stoi(token);
		}

		string message = ":hit " + std::to_string(damage);
		sessions[index]->sbuf = message;
		sessions[index]->sock->async_write_some(asio::buffer(sessions[index]->sbuf),
			bind(&Server::OnSend, this, _1));
		

	}

	void SetID(Session* session)
	{
		string temp = session->rbuf.substr(sizeof(":set ") - 1, session->rbuf.length());
		// �ߺ��� ���̵����� üũ
		for (int i = 0; i < sessions.size(); i++)
		{
			if (temp.compare(sessions[i]->id) == 0)
			{
				session->sbuf = "set falied: [" + temp + "]�� �̹� ������� ���̵� �Դϴ�";
				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
				return;
			}
		}

		session->id = temp;
		session->sbuf = "set [" + temp + "] success!";

		//���� ���� �÷��̾ �÷��̾� ��Ͽ� ����
		PlayerList.push_back(temp);
		//�÷��̾� ����� ����ȭ
		string ListSerial =":add "+ serializeStringArray(PlayerList);

		std::sort(sessions.begin(), sessions.end(), [](const Session* a, const Session* b) {
			return a->id < b->id;
			});

		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));




		if (session->room_no == -1)
		{
			session->room_no = 0;
			SendAll(session, 0, ListSerial, true);
		}
		else
		{
			SendAll(session, session->room_no, "[" + session->id + "] ���� ���̵� �����Ͽ����ϴ�", false);
		}
	}


	void SendAll(Session* session, int room_no, string message, bool sendToSenderAsWell)
	{
		// ���� �濡 �ִ� �ٸ� ��� Ŭ���̾�Ʈ�鿡�� ������
		for (int i = 0; i < sessions.size(); i++)
		{
			if ((session->sock != sessions[i]->sock) && (room_no == sessions[i]->room_no))
			{
				sessions[i]->sbuf = message;
				sessions[i]->sock->async_write_some(asio::buffer(sessions[i]->sbuf),
					bind(&Server::OnSend, this, _1));
			}
		}

		// �޼����� ������ Ŭ���̾�Ʈ���Ե� ������
		if (sendToSenderAsWell)
		{
			session->sbuf = message;
			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
		}
	}

	void OnSend(const system::error_code& ec)
	{
		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
			return;
		}
	}
	
	bool IsTheMessageInNumbers(string message)
	{
		const char* cTemp = message.c_str();

		// �޼��� ����(���ȣ)�� ������ �ƴ� ���
		for (int i = 0; i < message.length(); i++)
		{
			if (cTemp[i] < '0' || cTemp[i] > '9')
			{
				return false;
			}
		}

		return true;
	}


	void CloseSession(Session* session)
	{
		if (session->room_no != -1)
		{
			SendAll(session, 0, ":exit "+session->id, false);
		}
		// if session ends, close and erase
		for (int i = 0; i < sessions.size(); i++)
		{
			if (sessions[i]->sock == session->sock)
			{
				lock.lock();
				sessions.erase(sessions.begin() + i);
				lock.unlock();
				break;
			}
		}

		string temp = session->id;
		session->sock->close();

		auto it = std::remove(PlayerList.begin(), PlayerList.end(), temp);
		PlayerList.erase(it, PlayerList.end());

		delete session;
	}

	void SendReplication() {

		if (sessions.size()>=2) {

			std::vector<Replication> temp;
			for (int i = 0; i < sessions.size(); i++)
			{
				temp.push_back(sessions[i]->repli);
			}
			string Data = ":rep " + serializeReplicationArray(temp);
			for (int i = 0; i < sessions.size(); i++)
			{
				if (0 == sessions[i]->room_no)
				{
						sessions[i]->sbuf = Data;
						sessions[i]->sock->async_write_some(asio::buffer(sessions[i]->sbuf),
							bind(&Server::OnSend, this, _1));
				}
			}
	}
		// ���� Ÿ�̸� ���� �ð� ����
		replicationTimer.expires_at(replicationTimer.expires_at() + std::chrono::milliseconds(100));
		// ���� Ÿ�̸� ����
		replicationTimer.async_wait(boost::bind(&Server::SendReplication, this));
	}

	std::string serializeReplication(const Replication& rep) {
		std::stringstream ss;

		// PosX, PosY, PosZ, RotZ�� ���ڿ��� ����
		ss << rep.PosX << ' ' << rep.PosY << ' ' << rep.PosZ << ' ' << rep.RotZ << ' ';

		// State ����ü ���� int�� ��ȯ�Ͽ� ����
		ss << static_cast<int>(rep.state);

		return ss.str();
	}

	//�÷��̾� ��� ����ȭ
	std::string serializeStringArray(const std::vector<std::string>& strArray) {
		std::ostringstream oss;

		// �迭�� ũ�⸦ ���� ����
		oss << strArray.size() << " ";

		// �� ���ڿ��� ���ʴ�� ����
		for (const auto& str : strArray) {
			// ���ڿ� ���̸� ���� ����
			oss << str.length() << " " << str << " ";
		}

		return oss.str();
	}

	std::string serializeReplicationArray(const std::vector<Replication>& repArray) {
		std::stringstream ss;

		for (const auto& replication : repArray) {
			ss << serializeReplication(replication) << '\n'; // �� FReplication ��ü�� ����ȭ�Ͽ� ����
		}

		return ss.str();
	}
	Replication deserializeReplication(const std::string& data) {
		Replication replication;
		std::stringstream ss(data);
		// PosX, PosY, PosZ, RotZ�� ���ڿ����� �о����
		ss >> replication.PosX >> replication.PosY >> replication.PosZ >> replication.RotZ;

		// State ����ü ���� int�� �о�� �ٽ� ����ü ������ ��ȯ
		int stateValue;
		ss >> stateValue;
		replication.state = static_cast<State>(stateValue);

		return replication;
	}

};


int main()
{

	Server serv(asio::ip::address_v4::any().to_string(), 7777);
	serv.Start();
	
	return 0;
}