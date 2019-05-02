
#include "Header.h"
#include "Table.h"
#include <stack>
#include <sstream>
#include <set>
#include <iomanip>
Table::Table(SQL& sql)
{
    int i=3;
    int counter=0;//列数统计
    while(true)
    {
        if(sql[i]=="")
        {
            break;
        }
        else if(sql[i]=="PRIMARY")
        {
            auto pri=columns.find(sql[i+2]);
            primary=pri->second.order;
            pri_type=pri->second.type;
            i=i+3;
            continue;
        }
        col_info newc;
        newc.order=counter;
        counter++;
        string cname=sql[i++];
        col_name.push_back(cname);//记录列名顺序
        newc.type=sql[i++];
        if(sql[i]=="NOT"&&sql[i+1]=="NULL")
        {
            newc.not_null=true;
            i+=2;
        }
        else
            newc.not_null=false;
        columns[cname]=newc;
    }
    cnum=(int)columns.size();
    rnum=0;
}

void Table::insert_into(SQL &sql)
{
    int value_pos=0;
    for(int i=3;i<sql.get_size();i++)
    {
        if(sql[i]=="VALUES")
        {
            value_pos=i;
            break;
        }
    }
    string* temp=new string[cnum];
    for(int i=0;i<cnum;i++){
        temp[i]="NULL";
    }
    for(int i=3;i<value_pos;i++)
    {
        int c_pos=columns[sql[i]].order;
        temp[c_pos]=sql[i+value_pos-2];
    }
    for(int i=0;i<cnum;i++)
    {
        record.push_back(temp[i]);
    }
    rnum++;
    delete []temp;

}

bool Table::judge(string &str,int r) {//r是第几行 //计算每个表达式的正确性
    string op;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == '<' || str[i] == '=' || str[i] == '>') {
            op = str[i];
            str[i] = ' ';
            break;
        }
    }
    stringstream ss(str);
    string name, value;
    ss >> name >> value;//要比较的列名和数值
    int c = columns[name].order;
    if (columns[name].type =="INT" || columns[name].type == "DOUBLE") {//如果类型是int或double
        if (op =="<") {
            if (atof(record[r*cnum + c].c_str()) < atof(value.c_str())) return true;
            else return false;
        }
        else if (op == "=") {
            if (atof(record[r*cnum + c].c_str()) == atof(value.c_str())) return true;
            else return false;
        }
        else {
            if (atof(record[r*cnum + c].c_str()) > atof(value.c_str())) return true;
            else return false;
        }
    }
    else {//类型是CHAR
        if (op == "<") {
            if (record[r*cnum + c] < value) return true;
            else return false;
        }
        else if (op == "=") {
            if (record[r*cnum + c] == value) return true;
            else return false;
        }
        else {
            if (record[r*cnum + c] > value) return true;
            else return false;
        }
    }
}
void Table::where_clause(SQL &sql) {//where的位置
    int n;
    for(n=0;n<sql.get_size();n++)
    {
        if(sql[n]=="WHERE")
        {
            break;
        }
    }
    if(n==sql.get_size())
    {
        pick.clear();
        for(int lp=0;lp<rnum;lp++)
        {
            pick.push_back(true);
        }
        return;
    }
    string suff;//转后缀式
    map<string, int>p = { {"AND",1},{"OR",0} };
    stack<string> s;
    for (int i = n + 1; i < sql.get_size(); i++) {
        if (sql[i] == "AND" || sql[i] == "OR") {
            if (s.empty()) {
                s.push(sql[i]);
            }
            else {
                while (!s.empty()&&p[s.top()] > p[sql[i]]) {
                    suff += s.top();
                    suff += " ";
                    s.pop();
                }
                s.push(sql[i]);
            }
        }
        else {//不是运算符
            suff += sql[i];
            suff += " ";
        }
    }
    while (!s.empty()) {
        suff += s.top();
        suff += " ";
        s.pop();
    }
    //cout << suff << endl;//后缀式
    pick.clear();//清空原pick
    for (int i = 0; i < rnum; i++) {//每行循环
        stringstream ss(suff);
        string temp;
        stack<bool> cal;
        while (ss >> temp) {
            //cout<<temp<<endl;
            if (temp == "AND" || temp == "OR") {
                bool t2 = cal.top(); cal.pop();
                bool t1 = cal.top(); cal.pop();
                if (temp == "AND") {
                    if (t1&&t2) {
                        cal.push(true);
                    }
                    else cal.push(false);
                }
                else {
                    if (t1 || t2) {
                        cal.push(true);
                    }
                    else cal.push(false);
                }
            }
            else {
                cal.push(judge(temp, i));//第i行
                //cout<<"judge "<<(int)cal.top()<<endl;
            }
        }
        bool res = cal.top(); cal.pop();
        pick.push_back(res);
        //cout<<"push_back "<<(int)res<<endl;
    }
    /*for (int i = 0; i < rnum; i++) {
        if (pick[i]) cout <<"yes" << " ";
        else cout << "no" << " ";
    }
    cout << endl;*/
}


void Table::delete_from(SQL &sql)
{
    where_clause(sql);
    int pos=0;
    for(int i=0;i<rnum;i++)
    {
        if(pick[i])
        {
            for(int j=0;j<cnum;j++)
            {
                record.erase(record.begin()+pos);
                /*for(int i=0;i<(int)record.size();i++)
                {
                    cout<<record[i]<<' ';
                }
                cout<<endl;*/
            }
        }
        else
            pos+=cnum;
    }
    rnum--;
}
void Table::update(SQL &sql)
{
    where_clause(sql);
    int pos=3;
    while(sql[pos]!="WHERE")
    {
        string cname;
        int i=0;
        while(sql[pos][i]!='=')
        {
            cname+=sql[pos][i];
            i++;
        }
        string value;
        i++;
        while(i!=sql[pos].size())
        {
            value+=sql[pos][i];
            i++;
        }
        int c=columns[cname].order;
        for(int r=0;r<rnum;r++)
        {
            if(pick[r])
            {
                record[r*cnum+c]=value;
            }
        }
        pos++;
    }
}
void Table::select(SQL &sql)
{
    where_clause(sql);
    bool has_result=false;//判断是否有查询结果
    for(int i=0;i<rnum;i++)
    {
        if(pick[i])
            has_result=true;
    }
    if(!has_result)
        return;
    
    int pos=1;
    vector<bool> output;
    for(int lp=0;lp<cnum;lp++)
    {
        output.push_back(false);
    }
    while(sql[pos]!="FROM")
    {
        if(sql[pos]=="*")
        {
            for(int lp=0;lp<cnum;lp++)
            {
                output[lp]=true;
            }
            break;
        }
        else
        {
            output[columns[sql[pos]].order]=true;
        }
        pos++;
    }
    for(int c=0;c<cnum;c++)
    {
        if(output[c])
            cout<<col_name[c]<<'\t';
    }
    cout<<endl;
    
    if(pri_type=="CHAR"){//主键类型是char
    set<string> pri;//存主键
    for(int r=0;r<rnum;r++)
    {
        if(pick[r])
        {
            pri.insert(record[r*cnum+primary]);
        }//把要输出的主键放入
    }
    for(auto it=pri.begin();it!=pri.end();it++){
        for(int r=0;r<rnum;r++){
            if(*it==record[r*cnum+primary]){//说明输出第r行
                for(int c=0;c<cnum;c++){
                    if(output[c]){
                        if(columns[col_name[c]].type=="CHAR"){
                            if(record[r*cnum+c]=="NULL"){
                                cout<<"NULL\t";
                            }
                            else{string temp;
                            for(int i=1;i<record[r*cnum+c].size()-1;i++)
                            {
                                temp+=record[r*cnum+c][i];
                            }
                            cout<<temp<<'\t';
                            }
                        }
                        else if(columns[col_name[c]].type=="DOUBLE"){
                            if(record[r*cnum+c]=="NULL"){
                                cout<<"NULL\t";
                            }
                            else{cout<<fixed<<setprecision(4);
                            double x=atof(record[r*cnum+c].c_str());
                            cout<<x<<'\t';
                            //std::cout << std::defaultfloat;
                            }
                        }
                         else cout<<record[r*cnum+c]<<'\t'; 
                        }
                       
                }
            cout<<endl;
            break;
            }
        }
    }
    }
    else{//主键类型是int或double
    set<double> pri;
    for(int r=0;r<rnum;r++)
    {
        if(pick[r])
        {
            pri.insert(atof(record[r*cnum+primary].c_str()));
        }//把要输出的主键放入
    }
    for(auto it=pri.begin();it!=pri.end();it++){
        for(int r=0;r<rnum;r++){
            if(*it==atof(record[r*cnum+primary].c_str())){//说明输出第r行
                for(int c=0;c<cnum;c++){
                    if(output[c]){
                        if(columns[col_name[c]].type=="CHAR"){
                            if(record[r*cnum+c]=="NULL"){
                                cout<<"NULL\t";
                            }
                            else{string temp;
                            for(int i=1;i<record[r*cnum+c].size()-1;i++)
                            {
                                temp+=record[r*cnum+c][i];
                            }
                            cout<<temp<<'\t';
                            }
                        }
                        else if(columns[col_name[c]].type=="DOUBLE"){
                            if(record[r*cnum+c]=="NULL"){
                                cout<<"NULL\t";
                            }
                            else{cout<<fixed<<setprecision(4);
                            double x=atof(record[r*cnum+c].c_str());
                            cout<<x<<'\t';
                            //std::cout << std::defaultfloat;
                            }
                        }
                         else cout<<record[r*cnum+c]<<'\t'; 
                        }
                       
                }
            cout<<endl;
            break;
            }
        }
    }

    }
}

void Table::show_columns()
{
    cout<<"Field\tType\tNull\tKey\tDefault\tExtra\n";
    for(int i=0;i<cnum;i++)
    {
        auto it=columns.find(col_name[i]);
        cout<<it->first<<'\t';
        if(it->second.type=="INT")
        {
            cout<<"int(11)\t";
        }
        else if(it->second.type=="CHAR")
        {
            cout<<"char(1)\t";
        }
        else
            cout<<"double\t";
        if(it->second.not_null)
        {
            cout<<"NO"<<'\t';
        }
        else
            cout<<"YES"<<'\t';
        if(it->second.order==primary)
        {
            cout<<"PRI"<<'\t';
        }
        else
            cout<<'\t';
        cout<<"NULL"<<endl;
    }
}
