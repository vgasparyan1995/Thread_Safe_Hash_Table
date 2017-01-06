#pragma once

#include <iostream>
#include <unordered_map>
#include <set>
#include <thread>

#include "HashMap.h"

#define TEST(x, text) \
if ((x)) {\
    std::cout << "PASSED  " << text << std::endl;\
} else {\
    std::cout << "FAILED  " << text << std::endl;\
}

#define Container thread_safe::HashMap<int, char, 10>
#define LargeContainer thread_safe::HashMap<int, char, 1000>

void test_constructors()
{
    Container empty_cont;
    Container empty_cont_copy(empty_cont);
    Container empty_cont_copy_assign;
    empty_cont_copy_assign = empty_cont;
    TEST(empty_cont.empty() &&
         empty_cont_copy.empty() &&
         empty_cont_copy_assign.empty(),
         "Default constructor");

    std::unordered_map<int, char> temp_map = { {1, 'A'},
                                               {2, 'B'},
                                               {3, 'D'},
                                               {3, 'A'},
                                               {13, 'E'} };
    Container cont(temp_map.begin(), temp_map.end());
    Container cont_copy(cont);
    Container cont_copy_assign;
    cont_copy_assign = cont;
    TEST(cont.size() == temp_map.size() &&
         cont_copy.size() == temp_map.size() &&
         cont_copy_assign.size() == temp_map.size(),
         "Begin-end constructor");

    Container il_cont = { {1, 'A'},
                          {2, 'B'},
                          {3, 'D'},
                          {13, 'E'} };
    Container il_cont_copy(il_cont);
    Container il_cont_copy_assign;
    il_cont_copy_assign = il_cont;
    Container il_assign;
    il_assign = { {1, 'A'},
                  {2, 'B'},
                  {3, 'D'},
                  {13, 'E'} };
    TEST(il_cont.size() == 4 &&
         il_cont_copy.size() == 4 &&
         il_cont_copy_assign.size() == 4 &&
         il_assign.size() == 4,
         "Initializer list constructor");
    
    Container to_move_cont = { {1, 'A'},
                               {2, 'B'},
                               {3, 'D'},
                               {13, 'E'} };
    Container moved_cont(std::move(to_move_cont));
    Container move_assigned(std::move(moved_cont));
    TEST(move_assigned.size() == 4,
         "Move constructor");
}

void test_mutators()
{
    Container cont;
    cont.insert(1, 'A');
    cont.insert(std::make_pair(2, 'B'));
    TEST(cont.size() == 2, "Insert");

    auto insert_result = cont.insert(1, 'B');
    TEST(insert_result.second == false &&
         (*insert_result.first) == 'A',
         "Insert or Assign 1");
    auto insert_or_assign_result = cont.insert_or_assign(1, 'B');
    TEST((*insert_or_assign_result) == 'B' &&
         cont.size() == 2,
         "Insert or Assign 2");

    cont.erase(1);
    cont.erase(10);
    TEST(cont.size() == 1, "Erase 1");
    auto to_erase_result = cont.insert(1, 'B').first;
    cont.erase(to_erase_result);
    TEST(cont.size() == 1, "Erase 2");

    auto find2_result = cont.find(2);
    auto find2_result_other = cont.find(2);
    auto find1_result = cont.find(1);
    TEST(find1_result == cont.end() &&
         find2_result == find2_result_other &&
         (*find2_result) == 'B',
         "Find");

    auto ref = cont[2];
    TEST(ref == 'B', "operator[] 1");
    ref.set('A');
    TEST(ref == 'A', "operator[] 2");
    cont[3] = 'C';
    TEST(cont.size() == 2, "operator[] 3");

    cont.clear();
    TEST(cont.empty(), "Clear");
}

void test_selectors()
{
    Container cont;
    TEST(cont.empty(), "Empty 1");
    for (int i = 0; i < 1000; ++i) {
        cont.insert_or_assign(i, 'A');
    }
    TEST(!cont.empty() && cont.size() == 1000, "Size");
    cont.clear();
    TEST(cont.empty(), "Empty 2");
}

void test_iterators()
{
    std::unordered_map<int, char> um;
    Container hm;
    for (int i = 0; i < 1000; ++i) {
        um.insert(std::make_pair(i, 'A'));
        hm.insert(std::make_pair(i, 'A'));
    }
    std::set<std::pair<int, char> > umset, hmset;
    for (auto elem : um) {
        umset.insert(std::make_pair(elem.first, elem.second));
    }
    for (auto elem : hm) {
        hmset.insert(std::make_pair(elem.get_pair().first, elem.get_pair().second));
    }
    TEST(umset == hmset, "Begin to End");

    Container empty_cont;
    TEST(empty_cont.begin() == empty_cont.end(), "Begin of Empty");
}

void test()
{
    test_constructors();
    test_mutators();
    test_selectors();
    test_iterators();
}

#undef LargeContainer
#undef Container
#undef TEST
