# Quick Start - Testing Your Chat App

## Quick Test Run

1. **Start the server:**

   ```bash
   make server
   ./server 8080
   ```

2. **In a new terminal, run tests:**

   ```bash
   # Run all tests automatically
   ./run_tests.sh all

   # Or run specific tests
   ./run_tests.sh friends    # Friend service tests
   ./run_tests.sh groups     # Group service tests
   ./run_tests.sh complete   # Automated integration tests
   ```

## What Each Test Does

### 🧪 Friend Service Tests (`test_friends`)

**Duration: ~20 seconds**

Tests the friend system with two users (Alice and Bob):

1. Register and login both users
2. Alice sends friend request to Bob
3. Bob accepts the request
4. Both check their friend lists
5. Alice unfriends Bob
6. Test rejecting friend requests

**What you'll see:** Step-by-step output with full JSON requests/responses

---

### 🧪 Group Service Tests (`test_groups`)

**Duration: ~30 seconds**

Tests group functionality with multiple users:

1. Create users and login
2. Alice creates "Study Group"
3. Alice adds Bob and Charlie
4. Test permission system (non-owner can't add members)
5. Remove member and leave group operations
6. Owner leaves (dissolves group)

**What you'll see:** Detailed group operations with server responses

---

### 🧪 Complete Integration Tests (`test_complete`)

**Duration: ~40 seconds**

Automated test suite validating all features:

- ✅ 15+ automated test cases
- ✅ Account operations
- ✅ Friend operations
- ✅ Group operations
- ✅ Error handling
- ✅ Pass/fail summary report

**What you'll see:** Quick test execution with final summary:

```
Total Tests: 15
Passed: 14
Failed: 1
Success Rate: 93.33%
```

## Manual Testing

### Build tests manually:

```bash
make test-friends
make test-groups
make test-complete
```

### Run tests manually:

```bash
./test_friends 127.0.0.1 8080
./test_groups 127.0.0.1 8080
./test_complete 127.0.0.1 8080
```

## Using the Test Runner Script

The `run_tests.sh` script provides convenient commands:

```bash
./run_tests.sh build-all    # Build all test executables
./run_tests.sh friends       # Run friend tests
./run_tests.sh groups        # Run group tests
./run_tests.sh complete      # Run integration tests
./run_tests.sh all           # Run all tests sequentially
./run_tests.sh reset-db      # Reset database
./run_tests.sh clean         # Remove test binaries
./run_tests.sh help          # Show help
```

### Custom server address:

```bash
SERVER_HOST=192.168.1.100 SERVER_PORT=9000 ./run_tests.sh friends
```

## Troubleshooting

**"Cannot connect to server"**

- Make sure server is running: `./server 8080`
- Check the port number matches

**Tests fail with conflicts**

- Reset the database: `./run_tests.sh reset-db`
- Restart the server

**Compilation errors**

- Clean and rebuild: `make clean && make server`
- Build tests: `make build-tests`

## Next Steps

After running tests successfully:

1. Check [TESTING.md](TESTING.md) for detailed documentation
2. Review test source code to understand the protocol
3. Add your own custom tests
4. Test your new features

## Expected Output Example

```
╔════════════════════════════════════════╗
║   FRIEND SERVICE TEST SUITE            ║
╚════════════════════════════════════════╝

✅ Connected to server at 127.0.0.1:8080

==================================================
TEST 1: Register Users
==================================================

=== REGISTER: alice ===
📤 Sending: {
  "type": 1000,
  "data": {
    "username": "alice",
    "password": "password123"
  }
}
📥 Response: {
  "type": 2000,
  "data": {
    "status": 201,
    "message": "Registration successful",
    "user_id": 1
  }
}

... (more tests) ...

╔════════════════════════════════════════╗
║   ALL TESTS COMPLETED                  ║
╚════════════════════════════════════════╝
```

Enjoy testing! 🎉
