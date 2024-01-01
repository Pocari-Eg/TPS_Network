#include <iostream>
#include <boost/asio.hpp>
#include "/usr/include/mysql/mysql.h"

using namespace boost::asio;

using namespace std;
struct LoginStruct
{
    array<char, 25> id;
    array<char, 25> pwd;
};

struct JoinStruct
{
    array<char, 100> id;
    array<char, 100> pwd;
    array<char, 100> NickName;
};


#define DB_HOST "127.0.0.1"
#define DB_USER "admin"
#define DB_PWD "1eodnek1"
#define DB_NAME "TPSUser"
#define TB_NAME "UserTable"

LoginStruct Login_deserialize(const vector<char>& buffer);
JoinStruct Join_deserialize(const vector<char>& buffer);
bool CheckAlreayJoin(array<char, 100> id);
bool JoinAccount(JoinStruct UesrData);


int main() {
        io_context io_context;

        // ���� ����
        ip::tcp::acceptor acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), 8888));

        cout << "DB Server started. Listening on port 8888..." << endl;

        for (;;) {
            ip::tcp::socket socket(io_context);
            acceptor.accept(socket);

            // Ŭ���̾�Ʈ ���� ȹ��
            ip::tcp::endpoint clientEndpoint = socket.remote_endpoint();
            string clientIP = clientEndpoint.address().to_string();
            unsigned short clientPort = clientEndpoint.port();

            cout << "Client connected from " << clientIP << ":" << clientPort << endl;

            try {
                for (;;) {
                    // �����͸� �޾Ƽ� �ٽ� Ŭ���̾�Ʈ���� ����

                    vector<char> receivedData(sizeof(JoinStruct));
                    read(socket, buffer(receivedData));
                    JoinStruct receivedStruct = Join_deserialize(receivedData);


                    // Ŭ���̾�Ʈ�� ���� �޽��� ���
                    cout << "message from " << clientIP << ":" << clientPort << ": " << receivedStruct.id.data() << ","
                        << receivedStruct.pwd.data() << "," << receivedStruct.NickName.data() << endl;

                    

                    //true�̸� �����Ͱ� �����Ƿ� ���� �� �� �ִ�.
                    //false�̸� ������ �ȵǰų� �̹� ���������� �־� ������ �ȵȴ�.
                    if(!CheckAlreayJoin(receivedStruct.id))
                    {
                        string signal = "Error";
                        socket.write_some(buffer(signal));
                   
                    }

                    if (!JoinAccount(receivedStruct))
                    {
                        string signal = "Error";
                        socket.write_some(buffer(signal));
                    }
                    string signal = "Success";
                    // Ŭ���̾�Ʈ���� �޽��� �ٽ� ����
                    socket.write_some(buffer(signal));
                }
            }
            catch (exception& e) {
                // ���� �߻� �� ó��
                cerr << "Error: " << e.what() << endl;
            }
        }


    return 0;
}
#pragma Joinon serialize
LoginStruct Login_deserialize(const vector<char>& buffer) {
    LoginStruct result;
    memcpy(&result, buffer.data(), sizeof(result));
    return result;
}


JoinStruct Join_deserialize(const vector<char>& buffer) {
    JoinStruct result;
    memcpy(&result, buffer.data(), sizeof(result));
    return result;
}
#pragma endJoinon serialize

bool CheckAlreayJoin(array<char, 100> id)
{
    
    MYSQL* connection =NULL,conn;
    MYSQL_RES* sql_result;
    MYSQL_ROW sql_row;
    int query_stat;

   mysql_init(&conn);


   connection =mysql_real_connect(&conn,DB_HOST,DB_USER,DB_PWD,DB_NAME,3306,(char*)NULL,0);
    if(connection==NULL)
    {
        cout<<"mysql connect error : "<<mysql_error(&conn)<< endl;
        return false;
    }


    // Ư�� id�� ����Ͽ� ������ ��ȸ
    string query = "SELECT * FROM UserTable WHERE id = '";
    query += string(id.data()) + "'";
    if (mysql_query(&conn, query.c_str())) {
        cout << "Already join User" << endl;
        return false;
    }

  


    cout << "No Join User" << endl;

    //// MySQL ���� ����
    mysql_close(&conn);
    

    return true;
}
bool JoinAccount(JoinStruct UserData)
{


    MYSQL* connection = NULL, conn;
    MYSQL_RES* sql_result;
    MYSQL_ROW sql_row;
    int query_stat;

    mysql_init(&conn);


    connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PWD, DB_NAME, 3306, (char*)NULL, 0);
    if (connection == NULL)
    {
        cout << "mysql connect error : " << mysql_error(&conn) << endl;
        return false;
    }

    string m_id = "'" + string(UserData.id.data()) + "'";
    string m_pwd = "'" + string(UserData.pwd.data()) + "'";
    string m_NickName = "'" + string(UserData.NickName.data()) + "'";

   
    string query = "INSERT INTO UserTable (id, pwd, Nick, Lv) VALUES (";
    query += m_id + "," + m_pwd + "," + m_NickName + "," + "1"+")";



    if (mysql_query(&conn, query.c_str())) {
        cout << "Error Insert Data" << endl;
        cerr << "mysql_query() failed: " << mysql_error(&conn) << endl;
        return false;
    }

    cout << "Join User) ID :  " << m_id << endl;
    //// MySQL ���� ����
    mysql_close(&conn);
    return true;
}