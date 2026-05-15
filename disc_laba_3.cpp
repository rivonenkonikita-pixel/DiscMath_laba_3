#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cctype>
#include <cstdlib>

struct Node
{
    virtual ~Node() = default;
    virtual bool evaluate(const std::map<std::string, bool>& vars) const=0;
    virtual void collectVariables(std::set<std::string>& vars) const=0;
};

struct VariableNode: Node
{
    std::string name;
    VariableNode(const std::string& n): name(n) {}
    bool evaluate(const std::map<std::string, bool>& vars) const override
    {
        std::map<std::string, bool>::const_iterator it=vars.find(name);
        if (it==vars.end())
        {
            std::cerr<<"Ошибка: переменная "<<name<<" не найдена!"<<std::endl;
            exit(1);
        }
        return it->second;
    }
    void collectVariables(std::set<std::string>& vars) const override
    {
        vars.insert(name);
    }
};

struct UnaryOpNode: Node
{
    Node* expr;
    UnaryOpNode(Node* e): expr(e) {};
    ~UnaryOpNode() {delete expr;}
    bool evaluate(const std::map<std::string, bool>& vars) const override
    {
        return !expr->evaluate(vars);
    }
    void collectVariables(std::set<std::string>& vars) const override
    {
        expr->collectVariables(vars);
    }
};

struct BinaryOpNode: Node
{
    Node* left;
    Node* right;
    char operation;
    BinaryOpNode(Node* l, char op, Node* r): left(l), operation(op), right(r) {};
    ~BinaryOpNode() { delete left; delete right; }
    bool evaluate(const std::map<std::string, bool>& vars) const override
    {
        bool lv = left->evaluate(vars);
        bool rv = right->evaluate(vars);
        switch (operation)
        {
            case '+': return lv || rv;
            case '&': return lv && rv;
            case '@': return lv != rv;
            case '~': return lv == rv;
            case '>': return !lv || rv;
            case '|': return !(lv && rv);
            case '!': return !(lv || rv);
            default:
                std::cerr<<"Неизвестная операция: " << operation <<std::endl;
                exit(1);
        }
    }
    void collectVariables(std::set<std::string>& vars) const override
    {
        left->collectVariables(vars);
        right->collectVariables(vars);
    }
};

void DeleteNode(Node* node)
{
    if (!node) return;
    delete node;
}

class Parser
{
    public:
        Node* parse(const std::string& str)
        {
            position=0;
            Node* ast = parseExpression(str);
            if (ast && position!=str.size())
            {
                std::cerr<<"Ошибка: в конце строки есть лишние символы!"<<std::endl;
                DeleteNode(ast);
                return nullptr;
            }
            return ast;
        }
    private:
        size_t position = 0;
        bool isBinaryOp(char c) const
        {
            return c=='+' || c=='&' || c=='@' || c=='~' || c=='>' || c=='|' || c=='!';
        }
        void error(const std::string& msg)
        {
            std::cerr<<"Ошибка парсинга: "<<msg<<std::endl;
        }
        std::string parseVariable(const std::string& str)
        {
            if (position>=str.size() || !std::isalpha(static_cast<unsigned char>(str[position])))
            {
                error("должна быть буква!");
                return "";
            }
            std::string name;
            name+=str[position];
            position++;
            if (position>=str.size() || str[position]!='_')
            {
                error("после буквы должен быть символ '_'!");
                return "";
            }
            name+=str[position];
            position++;
            if (position>=str.size() || !std::isdigit(static_cast<unsigned char>(str[position])))
            {
                error("после '_' должна быть цифра!");
                return "";
            }
            while (position<str.size() && std::isdigit(static_cast<unsigned char>(str[position])))
            {
                name+=str[position];
                position++;
            }
            return name;
        }
        Node* parseExpression(const std::string& str)
        {
            if (position>=str.size())
            {
                error("формула оборвалась!");
                return nullptr;
            }
            char c=str[position];
            if (c=='-')
            {
                position++;
                Node* expr = parseExpression(str);
                if (!expr) return nullptr;
                return new UnaryOpNode(expr);
            }
            if (std::isalpha(static_cast<unsigned char>(c)))
            {
                std::string var = parseVariable(str);
                if (var.empty()) return nullptr;
                return new VariableNode(var);
            }
            if (c=='(')
            {
                position++;
                Node* left = parseExpression(str);
                if (!left) return nullptr;
                if (position>=str.size())
                {
                    error("должен быть оператор или ')'!");
                    DeleteNode(left);
                    return nullptr;
                }
                if (isBinaryOp(str[position]))
                {
                    char op = str[position];
                    position++;
                    Node* right = parseExpression(str);
                    if (!right)
                    {
                        DeleteNode(left);
                        return nullptr;
                    }
                    if (position>=str.size() || str[position]!=')')
                    {
                        error("должен быть символ ')'!");
                        DeleteNode(left);
                        DeleteNode(right);
                        return nullptr;
                    }
                    position++;
                    return new BinaryOpNode(left, op, right);
                }
                else if(str[position]==')')
                {
                    position++;
                    return left;
                }
                else
                {
                    error("должен быть оператор или ')'!");
                    DeleteNode(left);
                    return nullptr;
                }
            }
            error("некорректный символ!");
            return nullptr;
        }
};

//Удаление пробелов из строки с формулой
std::string removeSpaces(const std::string& str)
{
    std::string result;
    for (size_t i=0; i<str.size(); i++)
    {
        if (!isspace(static_cast<unsigned char>(str[i])))
        {
            result+=str[i];
        }
    }
    return result;
}

//Преобразование множества в вектор
std::vector<std::string> setToVector(const std::set<std::string>& str)
{
    std::vector<std::string> result;
    for (std::set<std::string>:: const_iterator it = str.begin(); it!=str.end(); it++)
    {
        result.push_back(*it);
    }
    return result;
}

int Position(const std::vector<std::string>& vec, const std::string& var)
{
    for (size_t i=0; i<vec.size(); i++)
    {
        if (vec[i]==var)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<bool> createTruthTable(const Node& root, const std::vector<std::string>& vars)
{
    size_t n = vars.size();
    size_t rows = 1<<n;
    std::vector<bool> table(rows);
    std::map<std::string, bool> env;
    for (size_t i=0; i<rows; i++)
    {
        for (size_t k=0; k<n; k++)
        {
            bool val = (i>>(n-1-k))&1;
            env[vars[k]]=val;
        }
        table[i]=root.evaluate(env);
    }
    return table;
}

void printTruthTable(const std::vector<std::string>& vars, const std::vector<bool>& table) {
    size_t n = vars.size();
    if (n == 0) {
        // Вывод для функции без переменных (константа)
        std::cout << "f" << std::endl;
        if (table.empty())
            std::cout << 0 << std::endl;
        else
            std::cout << table[0] << std::endl;
        return;
    }
    size_t rows = 1 << n;
    // Вывод заголовков переменных
    for (size_t i = 0; i < n; ++i)
        std::cout << vars[i] << "\t";
    std::cout << "f" << std::endl;
    // Вывод строк таблицы
    for (size_t i = 0; i < rows; ++i) {
        for (size_t k = 0; k < n; ++k) {
            bool val = (i >> (n - 1 - k)) & 1;
            std::cout << val << "\t";
        }
        std::cout << table[i] << std::endl;
    }
}

std::vector<bool> fictitiousVars(const std::vector<std::string>& vars, const std::vector<bool>& table)
{
    size_t n = vars.size();
    std::vector<bool> fictitious(n, true);
    if (n==0) return fictitious;
    for (size_t j=0; j<n; j++)
    {
        size_t mask = 1<<(n-1-j);
        bool fict_true = true;
        for (size_t i=0; i<(1<<n); i++)
        {
            if ((i&mask)==0)
            {
                if (table[i]!=table[i|mask])
                {
                    fict_true=false;
                    break;
                }
            }
        }
        fictitious[j]=fict_true;
    }
    return fictitious;
}

std::vector<bool> reduceTruthTable(const std::vector<std::string>& allVars, const std::vector<bool>& fulltable, const std::vector<std::string>& essentialVars)
{
    size_t n=allVars.size();
    size_t m=essentialVars.size();
    if (m==0) return std::vector<bool> (1, fulltable[0]);
    std::map<size_t, bool> mapping;
    for (size_t i = 0; i < (1 << n); ++i) {
        size_t idx = 0;
        for (size_t p = 0; p < m; ++p) {
            int origPos = Position(allVars, essentialVars[p]);
            if (origPos < 0) {
                std::cerr << "Ошибка: переменная не найдена" << std::endl;
                exit(1);
            }
            size_t mask = 1 << (n - 1 - origPos);
            int bit = (i & mask) ? 1 : 0;
            idx = (idx << 1) | bit;
        }
        mapping[idx] = fulltable[i];
    }
    std::vector<bool> reduced(1 << m);
    for (size_t i = 0; i < (1 << m); ++i) {
        reduced[i] = mapping[i];
    }
    return reduced;
}

//Построение СДНФ
std::string buildSDNF(const std::vector<std::string>& vars, const std::vector<bool>& table) {
    size_t m = vars.size(); //Количество переменных в функции
    if (m == 0) 
    {
        //Если переменных нет, то есть функция - константа
        if (table.empty()) 
        {
            return "0";
        } 
        else 
        { //Иначе возвращаем значение любой переменной, например первой
            if (table[0]) 
            {
                return "1";
            } 
            else 
            {
                return "0";
            }
        }
    }
    //В векторе terms мы будем хранить конъюнкции для СДНФ
    std::vector<std::string> terms;
    //Перебор всех наборов
    for (size_t i = 0; i < (1 << m); ++i) 
    {
        if (table[i]) 
        { //Если значение функции равно 1, то строим конъюнкцию
            std::string term = "(";
            for (size_t j = 0; j < m; ++j) 
            {
                bool bit = (i >> (m - 1 - j)) & 1;//Вычисляем значение переменной в наборе
                if (!bit) term += "-"; //Если переменная равна 0
                term += vars[j];
                if (j != m - 1) term += " & ";
            }
            term += ")";
            terms.push_back(term); //Добавляем в вектор
        }
    }
    if (terms.empty()) return "0"; //Нет коньюнкций
    std::string res;
    //Объединяем конъюнкции в строку
    for (size_t i = 0; i < terms.size(); ++i) 
    {
        if (i != 0) res += " + ";
        res += terms[i];
    }
    //Вернули СДНФ
    return res;
}

//Построение СКНФ
std::string buildSKNF(const std::vector<std::string>& vars, const std::vector<bool>& table) {
    size_t m = vars.size(); //Количество переменных в функции
    if (m == 0) 
    {
        //Если переменных нет, то есть функция - константа
        if (table.empty()) 
        {
            return "0";
        } 
        else 
        { //Иначе возвращаем значение любой переменной, например первой
            if (table[0]) 
            {
                return "1";
            } 
            else 
            {
                return "0";
            }
        }
    }
    //В векторе terms мы будем хранить дизъюнкции для СКНФ
    std::vector<std::string> terms;
    //Перебор всех наборов
    for (size_t i = 0; i < (1 << m); ++i) {
        if (!table[i]) { //Находим наборы, у которых значение функции равно 0
            std::string term = "(";
            for (size_t j = 0; j < m; ++j) {
                bool bit = (i >> (m - 1 - j)) & 1; //Вычисляем значение переменной в наборе
                if (bit) term += "-"; //Если переменная равна 1 добавляем знак отрицания
                term += vars[j]; //записываем переменную
                if (j != m - 1) term += " + ";
            }
            term += ")";
            terms.push_back(term); //добавляем дизъюнкцию
        }
    }
    //Если не нашли дизъюнкций, то есть функция тождественно положительна, то СКНФ=1
    if (terms.empty()) return "1";
    std::string res;
    //Записываем формулу СКНФ
    for (size_t i = 0; i < terms.size(); ++i) {
        if (i != 0) res += " & ";
        res += terms[i];
    }
    //вернули СКНФ
    return res;
}

//Построение АНФ
std::string buildANF(const std::vector<std::string>& vars, const std::vector<bool>& table) {
    size_t m = vars.size(); //Количество переменных в функции
    if (m == 0) 
    {
        //Если переменных нет, то есть функция - константа
        if (table.empty()) 
        {
            return "0";
        } 
        else 
        { //Иначе возвращаем значение любой переменной, например первой
            if (table[0]) 
            {
                return "1";
            } 
            else 
            {
                return "0";
            }
        }
    }
    std::vector<int> coeff(1 << m); //вектор с коэффициентами
    //Инициализация через таблицу истинности
    for (size_t i = 0; i < (1 << m); ++i) {
        if (table[i]) {
            coeff[i] = 1;
        } else {
            coeff[i] = 0;
        }
    }
    //Нахождение коэффициентов методом трегольника Паскаля
    for (size_t bit = 0; bit < m; ++bit) {
        size_t step = 1 << bit;
        for (size_t i = 0; i < (1 << m); i += (step << 1)) {
            for (size_t j = 0; j < step; ++j) {
                coeff[i + step + j] ^= coeff[i + j];
            }
        }
    }
    //Формирование мономов
    std::vector<std::string> monoms;
    //Пробег по всем наборам
    for (size_t i = 0; i < (1 << m); ++i) {
        if (coeff[i] == 1) {
            if (i == 0) {
                monoms.push_back("1"); //Самый первый коэффициент равен единице(особый случай)
            } else {
                std::string mon;
                for (size_t j = 0; j < m; ++j) {
                    if ((i >> (m - 1 - j)) & 1) {
                        if (!mon.empty()) mon += " & ";
                        mon += vars[j];
                    }
                }
                monoms.push_back(mon);
            }
        }
    }
    //Полином равен 0
    if (monoms.empty()) return "0";
    std::string res;
    //Записываем формулу АНФ
    for (size_t i = 0; i < monoms.size(); ++i) {
        if (i != 0) res += " @ ";
        res += monoms[i];
    }
    //Вернули АНФ
    return res;
}

//Двойственная функция
std::vector<bool> buildDualTable(const std::vector<bool>& table) {
    size_t rows = table.size(); //Количество строк в таблице истинности
    std::vector<bool> dual(rows); //Вектор хранит результаты двойственной функции
    for (size_t i = 0; i < rows; ++i) {
        size_t comp = (rows - 1) ^ i; //Побитовое инвертирование набора символов
        dual[i] = !table[comp]; //Значение двойственной функции
    }
    return dual; //Возвращаем результат
}

int main()
{
    const char* filename = "test.txt";
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return 1;
    }
    std::string formula, line;
    while (std::getline(file, line)) {
        formula += line + " ";
    }
    file.close();

    // Удаляем все пробелы
    std::string cleanFormula = removeSpaces(formula);
    if (cleanFormula.empty()) {
        std::cerr << "Ошибка: пустой файл" << std::endl;
        return 1;
    }

    // Парсинг
    Parser parser;
    Node* ast = parser.parse(cleanFormula);
    if (!ast) {
        // Сообщение об ошибке уже выведено в парсере
        return 1;
    }

    // Сбор переменных
    std::set<std::string> varSet;
    ast->collectVariables(varSet);
    std::vector<std::string> vars = setToVector(varSet);

    // Таблица истинности
    std::vector<bool> fullTable = createTruthTable(*ast, vars);
    std::cout << "Таблица истинности исходной функции:\n";
    printTruthTable(vars, fullTable);

    // Фиктивные переменные
    std::vector<bool> fictitious = fictitiousVars(vars, fullTable);
    std::vector<std::string> essentialVars;
    std::cout << "\nФиктивные переменные: ";
    bool hasFictitious = false;
    for (size_t i = 0; i < vars.size(); ++i) {
        if (fictitious[i]) {
            std::cout << vars[i] << " ";
            hasFictitious = true;
        } else {
            essentialVars.push_back(vars[i]);
        }
    }
    if (!hasFictitious) std::cout << "нет";
    std::cout << "\nСущественные переменные: ";
    for (size_t i = 0; i < essentialVars.size(); ++i)
    {
        std::cout << essentialVars[i];
        if (i!=essentialVars.size()-1)
        {
            std::cout<<", ";
        }
    }
    std::cout << "\n";

    // Функция без фиктивных переменных
    std::vector<bool> reducedTable = reduceTruthTable(vars, fullTable, essentialVars);
    std::cout << "\nТаблица истинности после удаления фиктивных переменных:\n";
    printTruthTable(essentialVars, reducedTable);

    // Нормальные формы
    std::string sdnf = buildSDNF(essentialVars, reducedTable);
    std::string sknf = buildSKNF(essentialVars, reducedTable);
    std::string anf  = buildANF(essentialVars, reducedTable);

    std::cout << "\nСДНФ: " << sdnf << "\n";
    std::cout << "СКНФ: " << sknf << "\n";
    std::cout << "АНФ:  " << anf << "\n";

    // Двойственная функция
    std::vector<bool> dualTable = buildDualTable(reducedTable);
    std::cout << "\nДвойственная функция (таблица истинности):\n";
    printTruthTable(essentialVars, dualTable);

    std::string dualSDNF = buildSDNF(essentialVars, dualTable);
    std::string dualSKNF = buildSKNF(essentialVars, dualTable);
    std::cout << "\nСДНФ двойственной функции: " << dualSDNF << "\n";
    std::cout << "СКНФ двойственной функции: " << dualSKNF << "\n";

    // Очистка дерева
    DeleteNode(ast);
    return 0;
}