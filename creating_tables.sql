CREATE TABLE users (
  	user_id SERIAL PRIMARY KEY,
  	name VARCHAR(100) NOT NULL,
	email VARCHAR(100) UNIQUE,
	role VARCHAR(20) NOT NULL,
	password_hash VARCHAR(255),
	);

CREATE TABLE products (
	product_id SERIAL PRIMARY KEY,
	name VARCHAR(100) NOT NULL,
	price DECIMAL(10,2) CHECK (price > 0),
	stock_quantity INTEGER CHECK (stock_quantity >= 0)
	);
	
CREATE TABLE orders (
	order_id SERIAL PRIMARY KEY,
	user_id INTEGER REFERENCES users(user_id),
	status VARCHAR(20) DEFAULT 'pending',
	total_price DECIMAL(10,2) DEFAULT 0,
	order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
	);

CREATE TABLE order_items (
	order_item_id SERIAL PRIMARY KEY,
	order_id INTEGER REFERENCES orders(order_id),
	product_id INTEGER REFERENCES products(product_id),
	quantity INTEGER CHECK (quantity > 0),
	price DECIMAL(10,2)
    );
	
CREATE TABLE IF NOT EXISTS order_status_history (
	history_id SERIAL PRIMARY KEY,
	order_id INTEGER REFERENCES orders(order_id),
	new_status VARCHAR(20),
	changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
	);
	
