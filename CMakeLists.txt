cmake_minimum_required(VERSION 4.0)
project(Financial_Transactions)

set(CMAKE_CXX_STANDARD 26)

include_directories(.
)
add_executable(Financial_Transactions main.cpp
        Bank_Account.cpp
        Transaction.h
        Income.h
        Expense.h)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL  https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)
add_executable(test_bank_account
        tests/test_bank_account.cpp
        Bank_Account.cpp
        Transaction.h
        Income.h
        Expense.h
)
target_link_libraries(test_bank_account
        gtest_main
)

add_test(NAME bank_account_test COMMAND test_bank_account)
