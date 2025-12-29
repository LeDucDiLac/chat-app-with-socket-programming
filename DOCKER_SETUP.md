# Docker Setup Guide

## Running Server in Docker, Client on Host

### Prerequisites

- Docker installed on VM/host
- Make sure port 5500 is not in use

### Step 1: Build and Run Server in Docker

```bash
# Option A: Using docker-compose (Recommended)
docker-compose up -d

# Option B: Using docker commands directly
docker build -t chat-server .
docker run -d -p 5500:5500 -v $(pwd)/database:/app/database --name chat-server chat-server
```

### Step 2: Initialize Database (First time only)

```bash
# Enter the running container
docker exec -it chat-server bash

# Inside container, build and run reset_db
cd /app
make reset-db
./reset-db

# Exit container
exit
```

### Step 3: Compile and Run Client on Host

```bash
# On host machine
make client

# Connect to server (use actual VM IP if running on different machine)
./client localhost 5500

# Or if server is on different machine:
./client <VM_IP_ADDRESS> 5500
```

## Useful Commands

### View server logs:

```bash
docker logs -f chat-server
```

### Stop server:

```bash
docker-compose down
# or
docker stop chat-server
```

### Restart server:

```bash
docker-compose restart
# or
docker restart chat-server
```

### Remove container and rebuild:

```bash
docker-compose down
docker-compose build --no-cache
docker-compose up -d
```

### Access container shell:

```bash
docker exec -it chat-server bash
```

## Troubleshooting

### Port already in use:

```bash
# Check what's using port 5500
sudo lsof -i :5500
# or
sudo netstat -tulpn | grep 5500
```

### Cannot connect from host:

1. Check container is running: `docker ps`
2. Check port mapping: `docker port chat-server`
3. Check firewall allows port 5500
4. Use correct IP address (VM's IP, not localhost if on different machines)

### Get VM IP address:

```bash
# On Linux/Mac
ip addr show
# or
hostname -I

# On Windows
ipconfig
```

## Network Configuration

### If server and client are on different machines:

1. Get VM's IP address (e.g., 192.168.1.100)
2. Ensure firewall allows incoming connections on port 5500
3. Connect client: `./client 192.168.1.100 5500`

### If using VM with NAT networking:

You may need to set up port forwarding in your VM settings:

- Host port: 5500
- Guest port: 5500
