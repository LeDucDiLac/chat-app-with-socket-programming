#!/bin/bash

# Chat App Test Runner
# This script helps you quickly build, run, and manage tests

set -e  # Exit on error

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SERVER_PORT=${SERVER_PORT:-8080}
SERVER_HOST=${SERVER_HOST:-127.0.0.1}

print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║   $1${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

show_usage() {
    cat << EOF
Usage: $0 [command]

Commands:
    build-all       Build all test executables
    friends         Run friend service tests
    groups          Run group service tests
    complete        Run complete integration test suite
    all             Run all tests sequentially
    clean           Clean all test binaries
    reset-db        Reset the database
    help            Show this help message

Environment Variables:
    SERVER_HOST     Server hostname (default: 127.0.0.1)
    SERVER_PORT     Server port (default: 8080)

Examples:
    $0 build-all
    SERVER_PORT=9000 $0 friends
    $0 all
EOF
}

check_server() {
    print_header "Checking Server Connection"
    
    # Check if server is running
    if nc -z $SERVER_HOST $SERVER_PORT 2>/dev/null; then
        print_success "Server is running on $SERVER_HOST:$SERVER_PORT"
        return 0
    else
        print_error "Cannot connect to server at $SERVER_HOST:$SERVER_PORT"
        print_warning "Please start the server first:"
        echo "  make server && ./server $SERVER_PORT"
        return 1
    fi
}

build_tests() {
    print_header "Building Test Executables"
    make build-tests
    print_success "All tests built successfully"
}

run_test() {
    local test_name=$1
    local test_bin=$2
    
    print_header "Running $test_name"
    
    if [ ! -f "$test_bin" ]; then
        print_error "Test binary not found: $test_bin"
        print_warning "Building tests first..."
        build_tests
    fi
    
    if check_server; then
        echo ""
        ./$test_bin $SERVER_HOST $SERVER_PORT
        echo ""
        print_success "$test_name completed"
    else
        return 1
    fi
}

run_friends_test() {
    run_test "Friend Service Tests" "test_friends"
}

run_groups_test() {
    run_test "Group Service Tests" "test_groups"
}

run_complete_test() {
    run_test "Complete Integration Tests" "test_complete"
}

run_all_tests() {
    print_header "Running All Tests"
    
    if ! check_server; then
        return 1
    fi
    
    echo ""
    run_friends_test
    echo ""
    sleep 2
    
    echo ""
    run_groups_test
    echo ""
    sleep 2
    
    echo ""
    run_complete_test
    echo ""
    
    print_success "All test suites completed!"
}

reset_database() {
    print_header "Resetting Database"
    
    if [ ! -f "reset-db" ]; then
        print_warning "Building reset-db..."
        make reset-db
    fi
    
    ./reset-db
    print_success "Database reset complete"
}

clean_tests() {
    print_header "Cleaning Test Binaries"
    rm -f test_friends test_groups test_complete
    print_success "Test binaries cleaned"
}

# Main script
case "${1:-help}" in
    build-all)
        build_tests
        ;;
    friends)
        run_friends_test
        ;;
    groups)
        run_groups_test
        ;;
    complete)
        run_complete_test
        ;;
    all)
        run_all_tests
        ;;
    reset-db)
        reset_database
        ;;
    clean)
        clean_tests
        ;;
    help|--help|-h)
        show_usage
        ;;
    *)
        print_error "Unknown command: $1"
        echo ""
        show_usage
        exit 1
        ;;
esac
