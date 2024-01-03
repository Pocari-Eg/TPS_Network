#include <iostream>
#include <boost/asio.hpp>
#include "/usr/include/mysql/mysql.h"

using namespace boost::asio;

using namespace std;


struct LoginStruct
{
    array<char, 25> id;
    array<char, 25> pwd;
    array<char, 25> NickName;
};


#define DB_HOST "127.0.0.1"
#define DB_USER "admin"
#define DB_PWD "1eodnek1"
#define DB_NAME "TPSUser"
#define TB_NAME "UserTable"

LoginStruct deserialize(const vector<char>& buffer);
bool CheckAlreayJoin(LoginStruct UserData);
bool JoinAccount(LoginStruct UesrData);

string Login_Join(LoginStruct UesrData, std::string NickName);

int idSize = 0;
int pwdSize = 0;
int nickNameSize = 0;
int main() 
{
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

  
                    vector<char> receivedData(sizeof(LoginStruct));
                    read(socket, buffer(receivedData));

                    LoginStruct receivedStruct = deserialize(receivedData);



                    std::string id_str(receivedStruct.id.begin(), receivedStruct.id.begin() + idSize);
                    std::string pwd_str(receivedStruct.pwd.begin(), receivedStruct.pwd.begin() + pwdSize);
                    std::string NickName_str(receivedStruct.NickName.begin(), receivedStruct.NickName.begin() + nickNameSize);

                        // Ŭ���̾�Ʈ�� ���� �޽��� ���
                   cout << "message from " << clientIP << ":" << clientPort << ": " << id_str << ", " << pwd_str << ", " << NickName_str << endl;

                   std::string signal=Login_Join(receivedStruct, NickName_str);

                   socket.write_some(buffer(signal));
                }
            }
            catch (exception& e) {
                // ���� �߻� �� ó��
                cerr << "Error: " << e.what() << endl;
                string signal = "Error";
                socket.write_some(buffer(signal));
            }
        }


    return 0;
}
#pragma region serialize

LoginStruct deserialize(const vector<char>& buffer) {
    LoginStruct result;

    idSize = 0;
    pwdSize = 0;
    nickNameSize = 0;

    for (auto iter = buffer.begin(); iter != buffer.begin() + result.id.size(); iter++) {
        if (*iter == '\0')break;
        idSize++;
    }

    memcpy(result.id.data(), buffer.data(), idSize);

    // Read pwd
    size_t pwdOffset = result.id.size();
    for (auto iter = buffer.begin() + pwdOffset; iter != buffer.begin() + pwdOffset + result.pwd.size(); iter++) {
        if (*iter == '\0')break;
        pwdSize++;
    }

    memcpy(result.pwd.data(), buffer.data() + pwdOffset, pwdSize);

    // Read NickName
    size_t nickNameOffset = pwdOffset + result.pwd.size();
    for (auto iter = buffer.begin() + nickNameOffset; iter != buffer.begin() + nickNameOffset + result.NickName.size(); iter++) {
        if (*iter == '\0')break;

        nickNameSize++;
    }

    memcpy(result.NickName.data(), buffer.data() + nickNameOffset, nickNameSize);

    return result;
}
#pragma endregion serialize


bool CheckAlreayJoin(LoginStruct UserData)
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

    std::string id_str(UserData.id.data(), UserData.id.data() + idSize);
    query += id_str + "'";

    cout << query << < endl;
    if (mysql_query(&conn, query.c_str())) {
        cout << "Already join User" << endl;
        return false;
    }

  


    cout << "No Join User" << endl;

    //// MySQL ���� ����
    mysql_close(&conn);
    

    return true;
}
bool JoinAccount(LoginStruct UserData)
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
    std::string id_str(UserData.id.data(), UserData.id.data() + idSize);
    std::string pwd_str(UserData.pwd.data(), UserData.pwd.data() + pwdSize);
    std::string NickName_str(UserData.NickName.data(), UserData.NickName.data() + nickNameSize);


    string m_id = "'" + id_str + "'";
    string m_pwd = "'" + pwd_str + "'";
    string m_NickName = "'" + NickName_str + "'";

   
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
string Login_Join(LoginStruct UesrData, std::string NickName)
{
    //���� �� ��
    if (NickName != "LOGIN") {

        //true�̸� �����Ͱ� �����Ƿ� ���� �� �� �ִ�.
        //false�̸� ������ �ȵǰų� �̹� ���������� �־� ������ �ȵȴ�.
        if (!CheckAlreayJoin(UesrData))
        {
            return "AlreayJoin";
        }

        if (!JoinAccount(UesrData))
        {
            return "JoinError";
        }
        return "Success";

    }
    //�α��� ��
    else {
        cout << "Login" << endl;
       return "Success";
    }
}