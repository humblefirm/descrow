#include <string>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <vector>
#include <descrow.hpp>
using namespace eosio;
using namespace std;
using namespace types;
class descrow : public contract
{
  public:
	descrow(account_name self)
		: contract(self) //,
						 //payq_repository(self, self)
	{
	}
	void request(const account_name shop, const account_name customer, const asset quantity, const string memo)
	{
		require_auth(shop);
		eosio_assert(shop != customer, "shop shouldn't be the same as chanllenger");
		payq_repository payq(_self, customer);

		auto itr = payq.find(shop);
		eosio_assert(itr == payq.end(), "queue already exists");

		print("\n>>> shop >>>", shop, " - shop: ", name{shop});
		print("\n>>> customer >>>", customer, " - customer: ", name{customer});

		eosio_assert(quantity.symbol == string_to_symbol(4, "COF"),
					 "This Contract only accepts COF for deposits");
		eosio_assert(quantity.is_valid(), "Invalid token transfer");

		payq.emplace(shop, [&](auto &r) {
			r.shop = shop;
			r.customer = customer;
			r.quantity = quantity;
			r.memo = memo;
		});

		payl_repository payl(_self, shop);
		auto itr_payl = payl.find(shop);
		if (itr_payl == payl.end())
			payl.emplace(shop, [&](auto &r) {
				r.owner = shop;
				r.balance.symbol=string_to_symbol(4, "COF");
				r.balance.amount=0;
			});
	}
	void claim(const account_name shop)
	{
		require_auth(shop);
		asset balance;
		payl_repository payl(_self, shop);
		auto itr_payl = payl.find(shop);
		eosio_assert(itr_payl != payl.end(), "Unknown account");

		action(
			permission_level{_self, N(active)},
			N(eosio.token), N(transfer),
			std::make_tuple(_self, shop, itr_payl->balance, std::string("")))
			.send();

		payl.erase(itr_payl);
	}

	void transfer(uint64_t sender, uint64_t receiver)
	{
		print("\n>>> sender >>>", sender, " - name: ", name{sender});
		print("\n>>> receiver >>>", receiver, " - name: ", name{receiver});
		// ??? Don't need to verify because we already did it in EOSIO_ABI_EX ???
		// eosio_assert(code == N(eosio.token), "I reject your non-eosio.token deposit");
		auto transfer_data = unpack_action_data<st_transfer>();
		if (transfer_data.from == _self || transfer_data.to != _self)
		{
			return;
		}
		print("\n>>> transfer data quantity >>> ", transfer_data.quantity);
		eosio_assert(transfer_data.quantity.symbol == string_to_symbol(4, "COF"),
					 "This Contract only accepts COF for deposits");
		eosio_assert(transfer_data.quantity.is_valid(), "Invalid token transfer");
		eosio_assert(transfer_data.quantity.amount > 0, "Quantity must be positive");
		//payq_repository payq(_self, transfer_data.from);
		payq_repository payq(_self, sender);
		auto itr_payq = payq.find(string_to_name(transfer_data.memo.c_str()));
		print("\n>>> shop >>>", name{itr_payq->shop}, " - memo: ", string_to_name(transfer_data.memo.c_str()));
		eosio_assert(itr_payq != payq.end(), "Wrong payment");
		eosio_assert(itr_payq->customer == transfer_data.from, "Wrong payment");

		payq.modify(itr_payq, sender, [&](auto &r) {
			// Assumption: total currency issued by eosio.token will not overflow asset
			r.quantity.amount -= transfer_data.quantity.amount;
			if (r.quantity.amount <= 0)
				r.status = 1;
		});
		asset new_balance;
		payl_repository payl(_self, string_to_name(transfer_data.memo.c_str()));
		auto itr_payl = payl.find(string_to_name(transfer_data.memo.c_str()));
		payl.modify(itr_payl, string_to_name(transfer_data.memo.c_str()), [&](auto &r) {
			// Assumption: total currency issued by eosio.token will not overflow asset
			r.balance += transfer_data.quantity;
			new_balance = r.balance;
		});

		print("\n", name{transfer_data.from}, " deposited:       ", transfer_data.quantity);
		print("\n", name{transfer_data.to}, " funds available: ", new_balance);
	}

	void close(const account_name &shop, const account_name &customer)
	{
		require_auth(shop);
		payq_repository payq(_self, customer);
		auto itr = payq.find(shop);
		eosio_assert(itr != payq.end(), "que doesn't exists");
		payq.erase(itr);
	}

	void deny(const account_name &shop, const account_name &customer)
	{
		require_auth(customer);
		payq_repository payq(_self, customer);
		auto itr = payq.find(shop);
		eosio_assert(itr != payq.end(), "que doesn't exists");
		payq.erase(itr);
	}
  private:
};
#undef EOSIO_ABI

#define EOSIO_ABI(TYPE, MEMBERS)                                                                                                 \
	extern "C"                                                                                                                   \
	{                                                                                                                            \
		void apply(uint64_t receiver, uint64_t code, uint64_t action)                                                            \
		{                                                                                                                        \
			if (action == N(onerror))                                                                                            \
			{                                                                                                                    \
				/* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */ \
				eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account");             \
			}                                                                                                                    \
			auto self = receiver;                                                                                                \
			if (code == self || code == N(eosio.token) || action == N(onerror))                                                  \
			{                                                                                                                    \
				TYPE thiscontract(self);                                                                                         \
				switch (action)                                                                                                  \
				{                                                                                                                \
					EOSIO_API(TYPE, MEMBERS)                                                                                     \
				}                                                                                                                \
				/* does not allow destructor of thiscontract to run: eosio_exit(0); */                                           \
			}                                                                                                                    \
		}                                                                                                                        \
	}

EOSIO_ABI(descrow, (request)(claim)(transfer)(close)(deny))
