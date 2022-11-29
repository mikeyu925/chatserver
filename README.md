### 聊天集群服务器项目——cpp







----

为了方便回顾自己写的项目，后面记录了写项目过程的一些记录。没有兴趣的话可以不用继续往下看了哈~



#### 数据库表设计

- User表  

  | 字段名称 | 字段类型                  | 字段说明     | 约束                        |
  | -------- | ------------------------- | ------------ | --------------------------- |
  | id       | INT                       | 用户id       | PRIMARY KEY、AUTO_INCREMENT |
  | name     | VARCHAR(50)               | 用户名       | NOT NULL, UNIQUE            |
  | password | VARCHAR(50)               | 用户密码     | NOT NULL                    |
  | state    | ENUM('online', 'offline') | 当前登录状态 | DEFAULT 'offline'           |

- Friend表

  | 字段名称 | 字段类型 | 字段说明 | 约束               |
  | -------- | -------- | -------- | ------------------ |
  | userid   | INT      | 用户id   | NOT NULL、联合主键 |
  | friendid | INT      | 好友id   | NOT NULL、联合主键 |

- AllGroup表

  | 字段名称  | 字段类型     | 字段说明   | 约束                        |
  | --------- | ------------ | ---------- | --------------------------- |
  | id        | INT          | 组id       | PRIMARY KEY、AUTO_INCREMENT |
  | groupname | VARCHAR(50)  | 组名称     | NOT NULL,UNIQUE             |
  | groupdesc | VARCHAR(200) | 分组的功能 | DEFAULT ''                  |

- GroupUser表

  | 字段名称  | 字段类型                  | 字段说明 | 约束               |
  | --------- | ------------------------- | -------- | ------------------ |
  | groupid   | INT                       | 组id     | NOT NULL、联合主键 |
  | userid    | INT                       | 组员id   | NOT NULL、联合主键 |
  | grouprole | ENUM('creator', 'normal') | 组内角色 | DEFAULT ‘normal’   |

- OfflineMessage表

  | 字段名称 | 字段类型     | 字段说明                   | 约束     |
  | -------- | ------------ | -------------------------- | -------- |
  | userid   | INT          | 用户id                     | NOT NULL |
  | message  | VARCHAR(500) | 离线消息（存储Json字符串） | NOT NULL |

