# cpp-flash-sale

一个用于逐步实现高并发秒杀流程的 C++17 学习项目。目前包含：

- 零第三方依赖的最小 HTTP 服务
- `GET /health` 健康检查接口
- Redis、MySQL、RabbitMQ 的 Docker Compose 开发环境
- 初始商品与订单表结构

## 本地编译

在 PowerShell 中执行：

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
./build/Debug/flash_sale.exe
```

服务默认监听 `8080` 端口。另开一个 PowerShell 验证：

```powershell
Invoke-RestMethod http://127.0.0.1:8080/health
```

预期响应：

```json
{"status":"ok","service":"cpp-flash-sale"}
```

可以通过环境变量修改端口：

```powershell
$env:PORT = "8090"
./build/Debug/flash_sale.exe
```

## 启动基础设施

安装并启动 Docker Desktop 后，在项目目录执行：

```powershell
Copy-Item .env.example .env
docker compose up -d
docker compose ps
```

开发环境端口：

| 服务 | 地址 |
| --- | --- |
| Redis | `127.0.0.1:6379` |
| MySQL | `127.0.0.1:3306` |
| RabbitMQ AMQP | `127.0.0.1:5672` |
| RabbitMQ 管理界面 | `http://127.0.0.1:15672` |

停止服务但保留数据：

```powershell
docker compose down
```

不要将 `.env` 提交到版本库。示例密码仅供本地开发使用。

## 下一步

1. 接入 Drogon，并把路由迁移到控制器。
2. 接入 Redis，完成库存预热和 Lua 原子预减。
3. 接入 RabbitMQ，发布异步下单消息。
4. 添加订单消费者与 MySQL 事务。
5. 增加幂等校验、失败补偿和压测脚本。
