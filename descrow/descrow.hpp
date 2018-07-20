#pragma once
#include <string>
#include <vector>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
using std::vector;
using std::map;
using std::string;
using namespace eosio;
namespace types {
	typedef uint64_t uuid;
	constexpr uint8_t  PET_TYPES = 109;
	constexpr uint32_t DAY = 86400;
	constexpr uint32_t HOUR = 3600;
	constexpr uint32_t MINUTE = 60;


	struct st_transfer {
		account_name from;
		account_name to;
		asset        quantity;
		string       memo;
	};
	// @abi table payl i64
	struct payl_table {
		account_name owner;
		asset    balance;
		
		uint64_t primary_key() const { return owner; }
		EOSLIB_SERIALIZE(payl_table, (owner)(balance))
	};
	typedef multi_index<N(payl), payl_table> payl_repository;


	// @abi table payq i64
	struct payq_table
	{
		account_name shop;
		account_name customer;
		//account_name turn;
		asset quantity;
		string memo;
		uint8_t status = 0;
		//vector<uint8_t> board;

		uint64_t primary_key() const { return shop; }
		EOSLIB_SERIALIZE(payq_table, (shop)(customer)(quantity)(memo)(status))
	};

	typedef multi_index<N(payq), payq_table> payq_repository;
}
