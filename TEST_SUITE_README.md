# Chat Application Test Suite

Comprehensive testing tools for your socket-based chat application.

## 📋 What's Included

This test suite provides three levels of testing:

| Test                  | File                | Purpose                          | Duration |
| --------------------- | ------------------- | -------------------------------- | -------- |
| **Friend Tests**      | `test_friends.cpp`  | Detailed friend service testing  | ~20s     |
| **Group Tests**       | `test_groups.cpp`   | Detailed group service testing   | ~30s     |
| **Integration Tests** | `test_complete.cpp` | Automated full system validation | ~40s     |

## 🚀 Quick Start

```bash
# 1. Build and start server
make server
./server 8080

# 2. In another terminal, run tests
./run_tests.sh all
```

That's it! The script will run all tests and show you the results.

## 📁 Files Created

```
chat-app-w-soket/
├── src/server/
│   ├── test_friends.cpp     ⭐ Friend service tests
│   ├── test_groups.cpp      ⭐ Group service tests
│   └── test_complete.cpp    ⭐ Integration test suite
├── run_tests.sh             ⭐ Test runner script
├── TESTING.md               📚 Detailed test documentation
├── QUICKSTART_TESTING.md    📚 Quick start guide
└── TEST_SUITE_README.md     📚 This file
```

## 🎯 Test Coverage

### ✅ Account Service

- User registration
- Login/logout
- Duplicate prevention
- Invalid credentials
- Session management

### ✅ Friend Service

- Send/accept/reject friend requests
- Get friend lists
- Unfriend operations
- Duplicate request prevention
- Cross-user validation

### ✅ Group Service

- Create/delete groups
- Add/remove members
- Permission checks
- Leave group operations
- Owner permissions

## 🛠️ Usage

### Using the Test Runner (Recommended)

```bash
./run_tests.sh [command]
```

**Commands:**

- `build-all` - Build all test executables
- `friends` - Run friend service tests
- `groups` - Run group service tests
- `complete` - Run integration tests
- `all` - Run all tests sequentially
- `reset-db` - Reset the database
- `clean` - Clean test binaries
- `help` - Show help

### Manual Testing

```bash
# Build tests
make test-friends
make test-groups
make test-complete

# Run tests
./test_friends 127.0.0.1 8080
./test_groups 127.0.0.1 8080
./test_complete 127.0.0.1 8080
```

### Custom Server

```bash
# Use different host/port
SERVER_HOST=192.168.1.100 SERVER_PORT=9000 ./run_tests.sh friends
```

## 📊 Example Output

### Verbose Tests (friends/groups)

```
╔════════════════════════════════════════╗
║   FRIEND SERVICE TEST SUITE            ║
╚════════════════════════════════════════╝

✅ Connected to server at 127.0.0.1:8080

==================================================
TEST 1: Register Users
==================================================

=== REGISTER: alice ===
📤 Sending: {"type":1000,"data":{"username":"alice"...}}
📥 Response: {"type":2000,"data":{"status":201...}}
```

### Automated Tests (complete)

```
✅ PASSED: Register User
✅ PASSED: Login with Valid Credentials
✅ PASSED: Send Friend Request
❌ FAILED: Invalid Test - Expected 200

╔════════════════════════════════════════╗
║        TEST SUMMARY                    ║
╚════════════════════════════════════════╝

Total Tests: 15
Passed: 14
Failed: 1
Success Rate: 93.33%
```

## 🔧 Building Tests

### Build All Tests

```bash
make build-tests
```

### Build Individual Tests

```bash
make test-friends   # Build friend tests
make test-groups    # Build group tests
make test-complete  # Build integration tests
```

### Clean Up

```bash
make clean          # Remove all binaries
./run_tests.sh clean  # Remove only test binaries
```

## 📖 Documentation

- **[QUICKSTART_TESTING.md](QUICKSTART_TESTING.md)** - Quick start guide for running tests
- **[TESTING.md](TESTING.md)** - Comprehensive test documentation with examples
- **[design.txt](design.txt)** - Protocol specification and API reference

## 🐛 Troubleshooting

**Problem:** Tests can't connect to server

```bash
# Solution: Make sure server is running
./server 8080
```

**Problem:** Test failures due to existing data

```bash
# Solution: Reset database
./run_tests.sh reset-db
# Or manually
make reset-db && ./reset-db
```

**Problem:** Compilation errors

```bash
# Solution: Clean and rebuild everything
make clean
make rebuild
make build-tests
```

**Problem:** Permission denied on run_tests.sh

```bash
# Solution: Make script executable
chmod +x run_tests.sh
```

## 🎓 Learning from Tests

The test files are excellent examples of:

- How to use the chat protocol
- JSON message formatting
- Client-server communication
- Error handling
- Multi-user scenarios

Feel free to:

- Read the test source code
- Modify tests for your needs
- Add new test cases
- Use them as examples for your own client

## 🔄 Continuous Testing

For development, you can:

1. **Run tests after each change:**

   ```bash
   make rebuild && ./run_tests.sh all
   ```

2. **Test specific features:**

   ```bash
   # After modifying friend service
   ./run_tests.sh friends
   ```

3. **Quick validation:**
   ```bash
   # Just run automated tests
   ./run_tests.sh complete
   ```

## 📝 Adding Your Own Tests

To add a new test function:

```cpp
void test_my_feature(int sock)
{
    std::cout << "\n=== MY TEST ===" << std::endl;

    json request = {
        {"type", YOUR_CMD_TYPE},
        {"data", {
            {"field", "value"}
        }}
    };

    send_and_receive(sock, request);
}
```

Then call it in your test's `main()` function.

## 🎉 Success!

If all tests pass, you'll see:

```
✅ All test suites completed!
```

Your chat application is working correctly! 🎊

---

**Need Help?** Check:

1. [QUICKSTART_TESTING.md](QUICKSTART_TESTING.md) - Step-by-step guide
2. [TESTING.md](TESTING.md) - Detailed documentation
3. Server logs - Check for error messages
4. Test output - Look for specific failure messages

Happy Testing! 🚀
