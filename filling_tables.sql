/* must be */
INSERT INTO users (name, email, role) VALUES 
    ('Админ', 'admin@shop.ru', 'admin'),
    ('Менеджер', 'manager@shop.ru', 'manager'),
    ('Покупатель', 'customer@shop.ru', 'customer') ON CONFLICT (email) DO NOTHING;
    );

/* additional */
INSERT INTO products(name, price, stock_quantity)
	  VALUES ('Сок Добрый Мультифрукт 1л', 139, 100);
