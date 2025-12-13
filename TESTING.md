# Test Suite Documentation

This directory contains comprehensive test scripts for the chat application's various services.

## Test Files

### 1. `test_friends.cpp` - Friend Service Tests

Tests all friend-related functionality including:

- ✅ Sending friend requests
- ✅ Accepting friend requests
- ✅ Rejecting friend requests
- ✅ Getting friend lists
- ✅ Unfriending users
- ✅ Duplicate request handling

**Usage:**

```bash
make test-friends
./test_friends 127.0.0.1 8080
```

### 2. `test_groups.cpp` - Group Service Tests

Tests all group-related functionality including:

- ✅ Creating groups
- ✅ Adding members to groups
- ✅ Removing members from groups
- ✅ Leaving groups
- ✅ Permission checks (owner vs member)
- ✅ Group dissolution when owner leaves

**Usage:**

```bash
make test-groups
./test_groups 127.0.0.1 8080
```

### 3. `test_complete.cpp` - Complete Integration Test

Comprehensive automated test suite that validates:

- ✅ Account operations (register, login, logout)
- ✅ Authentication edge cases (duplicate users, wrong passwords)
- ✅ Friend service operations
- ✅ Group service operations
- ✅ Permission and authorization checks
- ✅ Automated pass/fail tracking with summary report

**Usage:**

```bash
make test-complete
./test_complete 127.0.0.1 8080
```

## Building Tests

### Build Individual Test:

```bash
make test-friends    # Build friend service tests
make test-groups     # Build group service tests
make test-complete   # Build complete integration tests
```

### Build All Tests:

```bash
make build-tests
```

## Running Tests

**Prerequisites:**

1. Start the server on your chosen port:

   ```bash
   make server
   ./server 8080
   ```

2. Reset the database (optional, for clean test runs):

   ```bash
   make reset-db
   ./reset-db
   ```

3. Run the desired test:
   ```bash
   ./test_friends 127.0.0.1 8080
   ./test_groups 127.0.0.1 8080
   ./test_complete 127.0.0.1 8080
   ```

## Test Output

### Verbose Tests (test_friends.cpp, test_groups.cpp)

These tests provide detailed output showing:

- Each request sent to the server
- Full JSON request/response bodies
- Step-by-step test progression
- Clear section headers for each test phase

Example output:

```
╔════════════════════════════════════════╗
║   FRIEND SERVICE TEST SUITE            ║
╚════════════════════════════════════════╝

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
    "message": "Registration successful"
  }
}
```

### Automated Test (test_complete.cpp)

Provides pass/fail tracking with summary:

```
✅ PASSED: Register User
✅ PASSED: Reject Duplicate Registration
✅ PASSED: Login with Valid Credentials
❌ FAILED: Send Friend Request - Unexpected status

╔════════════════════════════════════════╗
║        TEST SUMMARY                    ║
╚════════════════════════════════════════╝

Total Tests: 15
Passed: 14
Failed: 1
Success Rate: 93.33%
```

## Test Coverage

### Account Service

- ✅ User registration
- ✅ Duplicate username prevention
- ✅ User login with valid credentials
- ✅ Invalid password rejection
- ✅ Non-existent user handling
- ✅ User logout
- ✅ Session management

### Friend Service

- ✅ Send friend request
- ✅ Duplicate friend request prevention
- ✅ Friend request to already-friends prevention
- ✅ Accept friend request
- ✅ Reject friend request
- ✅ Get friend list
- ✅ Unfriend operation
- ✅ Cross-user friend list validation

### Group Service

- ✅ Create group
- ✅ Add member to group (owner only)
- ✅ Permission denial for non-owners
- ✅ Remove member from group
- ✅ Member leaves group
- ✅ Owner leaves group (dissolves group)
- ✅ Multiple group operations

## Customizing Tests

All test files use a common pattern:

1. Connect to server
2. Create JSON request following the protocol
3. Send request and receive response
4. Validate response

You can easily add new tests by following this pattern:

```cpp
void test_my_feature(int sock)
{
    std::cout << "\n=== MY TEST ===" << std::endl;

    json request = {
        {"type", YOUR_COMMAND_TYPE},
        {"data", {
            {"field1", "value1"},
            {"field2", "value2"}
        }}
    };

    send_and_receive(sock, request);
}
```

## Protocol Reference

All tests follow the protocol defined in `design.txt`:

| Command Type | Name                  | Description              |
| ------------ | --------------------- | ------------------------ |
| 1000         | REGISTER              | Register new user        |
| 1001         | LOGIN                 | Login user               |
| 1002         | LOGOUT                | Logout user              |
| 1005         | SEND_FRIEND_REQUEST   | Send friend request      |
| 1006         | ACCEPT_FRIEND_REQUEST | Accept friend request    |
| 1007         | REJECT_FRIEND_REQUEST | Reject friend request    |
| 1008         | UNFRIEND              | Remove friendship        |
| 1009         | GET_FRIEND_LIST       | Get list of friends      |
| 1010         | CREATE_GROUP          | Create new group         |
| 1011         | ADD_TO_GROUP          | Add member to group      |
| 1012         | REMOVE_FROM_GROUP     | Remove member from group |
| 1013         | LEAVE_GROUP           | Leave group              |

## Troubleshooting

**Connection refused:**

- Make sure the server is running on the correct port
- Check firewall settings

**Test failures:**

- Reset the database between test runs for consistent state
- Ensure no duplicate usernames from previous runs
- Check server logs for errors

**Compilation errors:**

- Ensure all dependencies are installed (sqlite3, pthread)
- Check that the protocol.h and json.hpp headers are accessible

## Future Enhancements

Potential additions to the test suite:

- [ ] Message sending tests
- [ ] Offline message retrieval tests
- [ ] Concurrent user simulation
- [ ] Load testing
- [ ] Error recovery tests
- [ ] Database consistency tests
