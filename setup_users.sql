-- Create Users table
CREATE TABLE IF NOT EXISTS Users (
    user_id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(50) NOT NULL,
    role INT NOT NULL,  -- 1 for Librarian, 2 for Member
    member_id INT,
    FOREIGN KEY (member_id) REFERENCES Members(member_id)
);

-- Insert initial users
-- Librarian user
INSERT INTO Users (username, password, role) VALUES ('librarian', 'lib123', 1);

-- Member user
INSERT INTO Users (username, password, role) VALUES ('member', 'member123', 2); 