#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <pqxx/pqxx>

// Класс пользователя
class User {
protected: // Модификатор доступа: члены доступны в этом классе и в классах-наследниках
    int id;
    std::string name;
    std::string role;

public:
    // Инициализирует все поля при создании объекта
    User(int id, const std::string& name, const std::string& role)
        : id(id), name(name), role(role) {
    }
    //  this->id = id;     
    //  this->name = name;
    //  this->role = role;
    //  Это присваивание, а не инициализация: менее эффективно

    // Виртуальный деструктор
    // virtual - гарантирует правильное удаление объектов классов-наследников
    // = default - использует деструктор по умолчанию (автоматически сгенерированный компилятором)
    virtual ~User() = default;

    // Чисто виртуальная функция (абстрактный метод)
    // = 0 означает, что функция не имеет реализации в этом классе
    // Любой класс-наследник ДОЛЖЕН реализовать этот метод
    // Это делает класс User абстрактным - нельзя создать объект этого класса напрямую
    virtual void showMenu(pqxx::connection* conn) = 0;

    // Геттеры (методы доступа к приватным полям)
    // const в конце означает, что метод не изменяет состояние объекта
    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getRole() const { return role; }
};

// Классы-наследники
class Admin : public User {
public:
    Admin(int id, const std::string& name) : User(id, name, "admin") {}

    void showMenu(pqxx::connection* conn) override;
};

class Manager : public User {
public:
    Manager(int id, const std::string& name) : User(id, name, "manager") {}

    void showMenu(pqxx::connection* conn) override;
};

class Customer : public User {
public:
    Customer(int id, const std::string& name) : User(id, name, "customer") {}

    void showMenu(pqxx::connection* conn) override;
};

// Функции для админа
void adminAddProduct(pqxx::connection* conn) {
    std::string name;
    double price;
    int quantity;

    std::cout << "Название товара: ";
    std::getline(std::cin, name);
    std::cout << "Цена: ";
    std::cin >> price;
    std::cout << "Количество: ";
    std::cin >> quantity;
    std::cin.ignore();

    try {
        pqxx::work txn(*conn);
        txn.exec("INSERT INTO products (name, price, stock_quantity) VALUES (" +
            txn.quote(name) + ", " + txn.quote(price) + ", " +
            txn.quote(quantity) + ")");
        txn.commit();
        std::cout << "Товар добавлен\n";
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void adminUpdateProduct(pqxx::connection* conn) {
    int product_id;
    double price;
    int quantity;

    std::cout << "ID товара: ";
    std::cin >> product_id;
    std::cout << "Новая цена: ";
    std::cin >> price;
    std::cout << "Новое количество: ";
    std::cin >> quantity;
    std::cin.ignore();

    try {
        pqxx::work txn(*conn);
        txn.exec("UPDATE products SET price = " + txn.quote(price) +
            ", stock_quantity = " + txn.quote(quantity) +
            " WHERE product_id = " + txn.quote(product_id));
        txn.commit();
        std::cout << "Товар обновлен\n";
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void adminViewOrders(pqxx::connection* conn) {
    try {
        pqxx::work txn(*conn);
        auto result = txn.exec(
            "SELECT o.order_id, u.name as user_name, o.status, o.total_price, o.order_date "
            "FROM orders o "
            "JOIN users u ON o.user_id = u.user_id "
            "ORDER BY o.order_date DESC"
        );

        std::cout << "Все заказы:\n";
        for (auto row : result) {
            std::cout << "ID: " << row["order_id"].as<int>()
                << " | Клиент: " << row["user_name"].c_str()
                << " | Статус: " << row["status"].c_str()
                << " | Сумма: " << row["total_price"].as<double>()
                << " | Дата: " << row["order_date"].c_str() << "\n";
        }
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void adminChangeOrderStatus(pqxx::connection* conn) {
    int order_id;
    std::string status;

    std::cout << "ID заказа: ";
    std::cin >> order_id;
    std::cin.ignore();
    std::cout << "Новый статус (pending/processing/completed/canceled/returned): ";
    std::getline(std::cin, status);

    try {
        pqxx::work txn(*conn);
        // Сохраняем историю
        txn.exec(
            "INSERT INTO order_status_history (order_id, new_status) "
            "SELECT " + txn.quote(order_id) + ", " + txn.quote(status) + " "
            "WHERE NOT EXISTS ("
            "SELECT 1 FROM orders WHERE order_id = " + txn.quote(order_id) +
            " AND status = " + txn.quote(status) + ")"
        );

        // Меняем статус
        txn.exec("UPDATE orders SET status = " + txn.quote(status) +
            " WHERE order_id = " + txn.quote(order_id));
        txn.commit();
        std::cout << "Статус обновлен\n";
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

// Функции для менеджера
void managerViewPending(pqxx::connection* conn) {
    try {
        pqxx::work txn(*conn);
        auto result = txn.exec(
            "SELECT o.order_id, u.name, o.total_price, o.order_date "
            "FROM orders o "
            "JOIN users u ON o.user_id = u.user_id "
            "WHERE o.status = 'pending'"
        );

        std::cout << "Заказы в ожидании:\n";
        for (auto row : result) {
            std::cout << "ID: " << row["order_id"].as<int>()
                << " | Клиент: " << row["name"].c_str()
                << " | Сумма: " << row["total_price"].as<double>()
                << " | Дата: " << row["order_date"].c_str() << "\n";
        }
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void managerApproveOrder(pqxx::connection* conn) {
    int order_id;

    std::cout << "ID заказа для утверждения: ";
    std::cin >> order_id;
    std::cin.ignore();

    try {
        pqxx::work txn(*conn);
        txn.exec("UPDATE orders SET status = 'completed' "
            "WHERE order_id = " + txn.quote(order_id) +
            " AND status = 'processing'");

        // Сохраняем историю
        txn.exec(
            "INSERT INTO order_status_history (order_id, new_status) "
            "SELECT " + txn.quote(order_id) + ", 'completed' "
            "WHERE NOT EXISTS ("
            "SELECT 1 FROM orders WHERE order_id = " + txn.quote(order_id) +
            " AND status = 'comleted')"
        );
        txn.commit();
        std::cout << "Заказ утвержден\n";
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

// Функции для покупателя
void customerCreateOrder(pqxx::connection* conn, int user_id) {
    try {
        pqxx::work txn(*conn);

        // Создаем заказ
        auto result = txn.exec(
            "INSERT INTO orders (user_id, status, total_price) "
            "VALUES (" + txn.quote(user_id) + ", 'pending', 0) "
            "RETURNING order_id"
        );

        int order_id = result[0]["order_id"].as<int>();
        txn.commit();

        std::cout << "Заказ #" << order_id << " создан\n";

        // Добавляем товары
        char choice;
        do {
            int product_id, quantity;

            std::cout << "Добавить товар? (y/n): ";
            std::cin >> choice;
            std::cin.ignore();

            if (choice == 'y' || choice == 'Y') {
                std::cout << "ID товара: ";
                std::cin >> product_id;
                std::cout << "Количество: ";
                std::cin >> quantity;
                std::cin.ignore();

                pqxx::work txn2(*conn);
                // Проверяем наличие
                auto check = txn2.exec(
                    "SELECT stock_quantity, price FROM products "
                    "WHERE product_id = " + txn2.quote(product_id)
                );

                if (check.empty()) {
                    std::cout << "Товар не найден\n";
                    continue;
                }

                int stock = check[0]["stock_quantity"].as<int>();
                double price = check[0]["price"].as<double>();

                if (stock < quantity) {
                    std::cout << "Недостаточно товара. В наличии: " << stock << "\n";
                    continue;
                }

                // Добавляем в заказ
                txn2.exec(
                    "INSERT INTO order_items (order_id, product_id, quantity, price) "
                    "VALUES (" + txn2.quote(order_id) + ", " + txn2.quote(product_id) +
                    ", " + txn2.quote(quantity) + ", " + txn2.quote(price) + ")"
                );

                // Обновляем общую сумму
                txn2.exec(
                    "UPDATE orders SET total_price = ("
                    "SELECT SUM(quantity * price) FROM order_items "
                    "WHERE order_id = " + txn2.quote(order_id) + ") "
                    "WHERE order_id = " + txn2.quote(order_id)
                );

                // Уменьшаем количество на складе
                txn2.exec(
                    "UPDATE products SET stock_quantity = stock_quantity - " +
                    txn2.quote(quantity) + " WHERE product_id = " + txn2.quote(product_id)
                );

                txn2.commit();
                std::cout << "Товар добавлен в заказ\n";
            }
        } while (choice == 'y' || choice == 'Y');

    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void customerViewOrders(pqxx::connection* conn, int user_id) {
    try {
        pqxx::work txn(*conn);
        auto result = txn.exec(
            "SELECT order_id, status, total_price, order_date "
            "FROM orders WHERE user_id = " + txn.quote(user_id) +
            " ORDER BY order_date DESC"
        );

        std::cout << "Ваши заказы:\n";
        for (auto row : result) {
            std::cout << "Заказ #" << row["order_id"].as<int>()
                << " | Статус: " << row["status"].c_str()
                << " | Сумма: " << row["total_price"].as<double>()
                << " | Дата: " << row["order_date"].c_str() << "\n";

            // Показываем товары в заказе
            auto items = txn.exec(
                "SELECT p.name, oi.quantity, oi.price "
                "FROM order_items oi "
                "JOIN products p ON oi.product_id = p.product_id "
                "WHERE oi.order_id = " + txn.quote(row["order_id"].as<int>())
            );

            for (auto item : items) {
                std::cout << "  - " << item["name"].c_str()
                    << " x" << item["quantity"].as<int>()
                    << " = " << item["price"].as<double>() * item["quantity"].as<int>()
                    << " руб.\n";
            }
        }
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void customerPayOrder(pqxx::connection* conn, int user_id) {
    int order_id;

    std::cout << "ID заказа для оплаты: ";
    std::cin >> order_id;
    std::cin.ignore();

    try {
        pqxx::work txn(*conn);

        // Проверяем, что заказ принадлежит пользователю
        auto check = txn.exec(
            "SELECT status, total_price FROM orders "
            "WHERE order_id = " + txn.quote(order_id) +
            " AND user_id = " + txn.quote(user_id)
        );

        if (check.empty()) {
            std::cout << "Заказ не найден или не принадлежит вам\n";
            return;
        }

        std::string status = check[0]["status"].c_str();
        double total = check[0]["total_price"].as<double>();

        if (status != "pending") {
            std::cout << "Заказ уже оплачен или отменен\n";
            return;
        }

        std::cout << "Сумма к оплате: " << total << " руб.\n";
        std::cout << "Выберите способ оплаты:\n";
        std::cout << "1. Карта\n2. Кошелек\n3. СБП\n";

        int method;
        std::cin >> method;
        std::cin.ignore();

        std::string method_str;
        switch (method) {
        case 1: method_str = "card"; break;
        case 2: method_str = "wallet"; break;
        case 3: method_str = "sbp"; break;
        default: method_str = "unknown";
        }

        // Меняем статус заказа
        txn.exec(
            "UPDATE orders SET status = 'processing' "
            "WHERE order_id = " + txn.quote(order_id)
        );

        // Записываем оплату
        std::cout << "Оплата " << method_str << " на сумму " << total << " руб. прошла успешно!\n";

        txn.commit();

    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

// Общие функции
void viewProducts(pqxx::connection* conn) {
    try {
        pqxx::work txn(*conn);
        auto result = txn.exec("SELECT * FROM products ORDER BY name");

        std::cout << "Каталог товаров:\n";
        for (auto row : result) {
            std::cout << "ID: " << row["product_id"].as<int>()
                << " | " << row["name"].c_str()
                << " | Цена: " << row["price"].as<double>() << " руб."
                << " | В наличии: " << row["stock_quantity"].as<int>() << " шт.\n";
        }
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

// Менюшки
void Admin::showMenu(pqxx::connection* conn) {
    int choice;
    do {
        std::cout << "\n\tМЕНЮ АДМИНИСТРАТОРА\n";
        std::cout << "1. Добавить товар\n";
        std::cout << "2. Обновить товар\n";
        std::cout << "3. Просмотр всех заказов\n";
        std::cout << "4. Изменить статус заказа\n";
        std::cout << "5. Просмотр товаров\n";
        std::cout << "0. Выход\n";
        std::cout << "Выбор: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
        case 1: adminAddProduct(conn); break;
        case 2: adminUpdateProduct(conn); break;
        case 3: adminViewOrders(conn); break;
        case 4: adminChangeOrderStatus(conn); break;
        case 5: viewProducts(conn); break;
        case 0: std::cout << "Выход из системы\n"; break;
        default: std::cout << "Неверный выбор\n";
        }
    } while (choice != 0);
}

void Manager::showMenu(pqxx::connection* conn) {
    int choice;
    do {
        std::cout << "\n\tМЕНЮ МЕНЕДЖЕРА\n";
        std::cout << "1. Заказы в ожидании\n";
        std::cout << "2. Утвердить заказ\n";
        std::cout << "3. Просмотр товаров\n";
        std::cout << "0. Выход\n";
        std::cout << "Выбор: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
        case 1: managerViewPending(conn); break;
        case 2: managerApproveOrder(conn); break;
        case 3: viewProducts(conn); break;
        case 0: std::cout << "Выход из системы\n"; break;
        default: std::cout << "Неверный выбор\n";
        }
    } while (choice != 0);
}

void Customer::showMenu(pqxx::connection* conn) {
    int choice;
    do {
        std::cout << "\n\tМЕНЮ ПОКУПАТЕЛЯ\n";
        std::cout << "1. Создать заказ\n";
        std::cout << "2. Мои заказы\n";
        std::cout << "3. Оплатить заказ\n";
        std::cout << "4. Просмотр товаров\n";
        std::cout << "0. Выход\n";
        std::cout << "Выбор: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
        case 1: customerCreateOrder(conn, getId()); break;
        case 2: customerViewOrders(conn, getId()); break;
        case 3: customerPayOrder(conn, getId()); break;
        case 4: viewProducts(conn); break;
        case 0: std::cout << "Выход из системы\n"; break;
        default: std::cout << "Неверный выбор\n";
        }
    } while (choice != 0);
}

int main() {
    try {
        // Подключение к БД
        std::string conn_str = "dbname=shop user=postgres password=20Dec@07$ host=localhost";
        pqxx::connection conn(conn_str);

        if (!conn.is_open()) {
            std::cout << "Ошибка подключения к БД\n";
            return 1;
        }

        std::cout << "Подключено к базе данных интернет-магазина\n";

        int choice;
        do {
            std::cout << "\n\tИНТЕРНЕТ-МАГАЗИН 'ООО Тмыв'\n";
            std::cout << "1. Войти как Администратор\n";
            std::cout << "2. Войти как Менеджер\n";
            std::cout << "3. Войти как Покупатель\n";
            std::cout << "0. Выход\n";
            std::cout << "Выбор: ";
            std::cin >> choice;
            std::cin.ignore();

            std::unique_ptr<User> currentUser;

            switch (choice) {
            case 1: {
                currentUser = std::make_unique<Admin>(1, "Админ");
                currentUser->showMenu(&conn);
                break;
            }
            case 2: {
                currentUser = std::make_unique<Manager>(2, "Менеджер");
                currentUser->showMenu(&conn);
                break;
            }
            case 3: {
                currentUser = std::make_unique<Customer>(3, "Покупатель");
                currentUser->showMenu(&conn);
                break;
            }
            case 0: std::cout << "До свидания!\n"; break;
            default: std::cout << "Неверный выбор\n";
            }

        } while (choice != 0);

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
