// Unit tests for LEZCoreModule.
// All wallet_ffi C functions are mocked at link time via mock_wallet_ffi.cpp.

#include <logos_test.h>
#include "lez_core_module.h"
#include "mocks/mock_wallet_ffi_capture.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// 64-char hex string = 32 bytes (valid account id).
static const std::string VALID_ID = std::string(64, 'a');
static const std::string VALID_ID_2 = std::string(64, 'b');
// 32-char hex string = 16 bytes (valid amount / solution).
static const std::string VALID_U128 = std::string(32, '1');

// The impl returns serialized JSON strings (Qt-free). Parse them here so tests
// can assert on individual fields.
static nlohmann::json parseObject(const std::string& json) {
    return nlohmann::json::parse(json, nullptr, false);
}

// ============================================================================
// Plugin metadata
// ============================================================================

LOGOS_TEST(name_and_version) {
    LEZCoreModule module;
    LOGOS_ASSERT_EQ(module.name(), std::string("lez_core"));
    LOGOS_ASSERT_EQ(module.version(), std::string("0.4.0-alpha.2"));
}

// ============================================================================
// Account management
// ============================================================================

LOGOS_TEST(create_account_public_returns_hex_on_success) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string id = module.create_account_public();
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_create_account_public"));
    // Mock fills the id with 0xAB bytes -> 64 hex chars ("ab" x 32).
    std::string expected;
    for (int i = 0; i < 32; ++i) expected += "ab";
    LOGOS_ASSERT_EQ(id, expected);
}

LOGOS_TEST(create_account_public_returns_empty_on_error) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_create_account_public").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.create_account_public().empty());
}

LOGOS_TEST(create_account_private_returns_hex_on_success) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string id = module.create_account_private();
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_create_account_private"));
    std::string expected;
    for (int i = 0; i < 32; ++i) expected += "cd";
    LOGOS_ASSERT_EQ(id, expected);
}

LOGOS_TEST(list_accounts_maps_entries) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("list_accounts_count").returns(3);
    LEZCoreModule module;

    const LogosList accounts = module.list_accounts();
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_list_accounts"));
    LOGOS_ASSERT_EQ(static_cast<int>(accounts.size()), 3);

    // entry 0 is public, entry 1 is private.
    std::string expectedId0;
    for (int i = 0; i < 32; ++i) expectedId0 += "10";
    LOGOS_ASSERT_TRUE(accounts[0]["is_public"].get<bool>());
    LOGOS_ASSERT_EQ(accounts[0]["account_id"].get<std::string>(), expectedId0);
    LOGOS_ASSERT_FALSE(accounts[1]["is_public"].get<bool>());
}

LOGOS_TEST(list_accounts_empty_on_error) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_list_accounts").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(static_cast<int>(module.list_accounts().size()), 0);
}

// ============================================================================
// Account queries
// ============================================================================

LOGOS_TEST(get_balance_invalid_hex_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.get_balance("not-hex", true).empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_get_balance"));
}

LOGOS_TEST(get_balance_returns_decimal_string) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("get_balance_value").returns(123456789);
    LEZCoreModule module;

    const std::string balance = module.get_balance(VALID_ID, true);
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_get_balance"));
    LOGOS_ASSERT_EQ(balance, std::string("123456789"));
}

LOGOS_TEST(get_balance_zero) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("get_balance_value").returns(0);
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.get_balance(VALID_ID, false), std::string("0"));
}

LOGOS_TEST(get_account_public_returns_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.get_account_public(VALID_ID));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_get_account_public"));
    // program_owner mocked to 0xAA bytes.
    std::string expectedOwner;
    for (int i = 0; i < 32; ++i) expectedOwner += "aa";
    LOGOS_ASSERT_EQ(obj["program_owner"].get<std::string>(), expectedOwner);
}

LOGOS_TEST(get_account_public_invalid_hex_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.get_account_public("zz").empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_get_account_public"));
}

LOGOS_TEST(get_public_account_key_returns_hex) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    std::string expected;
    for (int i = 0; i < 32; ++i) expected += "be";
    LOGOS_ASSERT_EQ(module.get_public_account_key(VALID_ID), expected);
}

LOGOS_TEST(get_private_account_keys_returns_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.get_private_account_keys(VALID_ID));
    std::string expected;
    for (int i = 0; i < 32; ++i) expected += "ef";
    LOGOS_ASSERT_EQ(obj["nullifier_public_key"].get<std::string>(), expected);
}

// ============================================================================
// Account encoding
// ============================================================================

LOGOS_TEST(account_id_to_base58_invalid_hex_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.account_id_to_base58("xyz").empty());
}

LOGOS_TEST(account_id_to_base58_returns_string) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_account_id_to_base58").returns("SomeBase58Value");
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.account_id_to_base58(VALID_ID), std::string("SomeBase58Value"));
}

LOGOS_TEST(account_id_from_base58_returns_hex) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    std::string expected;
    for (int i = 0; i < 32; ++i) expected += "5a";
    LOGOS_ASSERT_EQ(module.account_id_from_base58("anything"), expected);
}

LOGOS_TEST(account_id_from_base58_error_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_account_id_from_base58").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.account_id_from_base58("anything").empty());
}

// ============================================================================
// Blockchain synchronisation
// ============================================================================

LOGOS_TEST(sync_to_block_forwards_value) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_sync_to_block").returns(7);
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.sync_to_block(100), static_cast<int64_t>(7));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_sync_to_block"));
}

LOGOS_TEST(get_last_synced_block_returns_value) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("last_synced_block_value").returns(55);
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.get_last_synced_block(), static_cast<int64_t>(55));
}

LOGOS_TEST(get_last_synced_block_error_returns_negative_sentinel) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_get_last_synced_block").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.get_last_synced_block(), static_cast<int64_t>(-1));
}

LOGOS_TEST(get_current_block_height_returns_value) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("current_block_height_value").returns(999);
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.get_current_block_height(), static_cast<int64_t>(999));
}

LOGOS_TEST(get_current_block_height_error_returns_negative_sentinel) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_get_current_block_height").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.get_current_block_height(), static_cast<int64_t>(-1));
}

LOGOS_TEST(get_local_public_block_history_uses_configured_wallet_leader) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_open").returns(1);
    t.mockCFunction("local_public_block_history_json").returns(
        "{\"snapshot_tip\":{\"block_id\":17},\"blocks\":[],\"next_block_id\":null}");
    LEZCoreModule module;
    LOGOS_ASSERT_EQ(module.open("/cfg", "/store"), static_cast<int64_t>(SUCCESS));

    const std::string response = module.get_local_public_block_history(
        18,
        "{\"block_id\":17,\"block_hash\":\""
        + std::string(64, 'a')
        + "\",\"previous_block_hash\":\""
        + std::string(64, 'b') + "\"}");

    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_get_local_public_block_history"));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_free_string"));
    LOGOS_ASSERT_EQ(
        MockWalletFfiCapture::lastLocalHistoryStartBlockId,
        static_cast<uint64_t>(18));
    LOGOS_ASSERT_TRUE(MockWalletFfiCapture::lastLocalHistoryHasExpectedTip);
    LOGOS_ASSERT_EQ(
        MockWalletFfiCapture::lastLocalHistoryExpectedTipBlockId,
        static_cast<uint64_t>(17));
    LOGOS_ASSERT_EQ(
        MockWalletFfiCapture::lastLocalHistoryBlockHash[0],
        static_cast<uint8_t>(0xaa));
    LOGOS_ASSERT_EQ(
        MockWalletFfiCapture::lastLocalHistoryPreviousBlockHash[0],
        static_cast<uint8_t>(0xbb));
    LOGOS_ASSERT_EQ(
        response,
        std::string("{\"snapshot_tip\":{\"block_id\":17},\"blocks\":[],\"next_block_id\":null}"));
}

LOGOS_TEST(get_local_public_block_history_rejects_invalid_cursor_before_ffi) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_open").returns(1);
    LEZCoreModule module;
    LOGOS_ASSERT_EQ(module.open("/cfg", "/store"), static_cast<int64_t>(SUCCESS));

    LOGOS_ASSERT_TRUE(
        module.get_local_public_block_history(-1, std::string()).empty());
    LOGOS_ASSERT_TRUE(
        module.get_local_public_block_history(1, "{\"block_id\":0}").empty());
    LOGOS_ASSERT_TRUE(
        module.get_local_public_block_history(1, "{\"block_id\":-1,\"block_hash\":\""
            + std::string(64, 'a')
            + "\",\"previous_block_hash\":\""
            + std::string(64, 'b') + "\"}").empty());
    LOGOS_ASSERT_FALSE(
        t.cFunctionCalled("wallet_ffi_get_local_public_block_history"));
}

// ============================================================================
// Transfers / registration
// ============================================================================

LOGOS_TEST(transfer_public_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.transfer_public(VALID_ID, VALID_ID_2, VALID_U128));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_transfer_public"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
    LOGOS_ASSERT_EQ(obj["tx_hash"].get<std::string>(), std::string("0xmocktxhash"));
    LOGOS_ASSERT_TRUE(obj["error"].get<std::string>().empty());
}

LOGOS_TEST(transfer_public_invalid_hex_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.transfer_public("bad", VALID_ID_2, VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_transfer_public"));
}

LOGOS_TEST(transfer_public_invalid_amount_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.transfer_public(VALID_ID, VALID_ID_2, "ff"));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_CONTAINS(obj["error"].get<std::string>(), std::string("amount"));
}

LOGOS_TEST(transfer_public_ffi_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_transfer_public").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.transfer_public(VALID_ID, VALID_ID_2, VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
}

LOGOS_TEST(transfer_public_uses_deployed_testnet_program) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_get_sequencer_addr").returns("https://testnet.lez.logos.co/");
    LEZCoreModule module;

    const std::string amount = "00ffeeddccbbaa998877665544332211";
    const nlohmann::json obj = parseObject(module.transfer_public(VALID_ID, VALID_ID_2, amount));

    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_send_generic_public_transaction"));
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_transfer_public"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
    LOGOS_ASSERT_EQ(obj["tx_hash"].get<std::string>(), std::string("0xmocktxhash2"));
    const std::vector<uint32_t> expected_words = {
        0,
        0xddeeff00,
        0x99aabbcc,
        0x55667788,
        0x11223344,
    };
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicInstructionWords.size(), expected_words.size());
    for (size_t index = 0; index < expected_words.size(); ++index) {
        LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicInstructionWords[index], expected_words[index]);
    }
    const std::array<uint8_t, 32> expected_program_id = {
        0xdc, 0xbb, 0xfe, 0xbc, 0xd5, 0x93, 0x99, 0x96,
        0x1e, 0xd9, 0x97, 0x3b, 0x83, 0x07, 0xdc, 0x47,
        0x5f, 0xd4, 0xc5, 0xca, 0x57, 0x79, 0xaa, 0xcf,
        0xe7, 0x58, 0x8f, 0x7d, 0xbc, 0x3f, 0x4a, 0x71,
    };
    LOGOS_ASSERT(MockWalletFfiCapture::lastGenericPublicProgramId == expected_program_id);
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountIds.size(), static_cast<size_t>(2));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountKinds[0], static_cast<int>(FfiAccountIdentityKind::PUBLIC));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountKinds[1], static_cast<int>(FfiAccountIdentityKind::PUBLIC_NO_SIGN));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountIds[0][0], static_cast<uint8_t>(0xaa));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountIds[1][0], static_cast<uint8_t>(0xbb));
}

LOGOS_TEST(transfer_shielded_invalid_keys_json_error) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    // to_keys_json is not valid JSON object -> parse failure.
    const nlohmann::json obj = parseObject(module.transfer_shielded(VALID_ID, "not-json", VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_transfer_shielded"));
}

LOGOS_TEST(transfer_shielded_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string keysJson = std::string("{\"nullifier_public_key\":\"") + std::string(64, 'a') + std::string("\"}");
    const nlohmann::json obj = parseObject(module.transfer_shielded(VALID_ID, keysJson, VALID_U128));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_transfer_shielded"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
}

// to_keys_json carrying an explicit "identifier" must be forwarded to the FFI call as-is.
LOGOS_TEST(transfer_shielded_with_identifier_forwards_it_unchanged) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string identifierHex(32, '7'); // 16 bytes of 0x77
    const std::string keysJson = std::string("{\"nullifier_public_key\":\"") + std::string(64, 'a')
        + "\",\"identifier\":\"" + identifierHex + "\"}";

    const nlohmann::json obj = parseObject(module.transfer_shielded(VALID_ID, keysJson, VALID_U128));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());

    uint8_t expected[16];
    memset(expected, 0x77, sizeof(expected));
    LOGOS_ASSERT(memcmp(MockWalletFfiCapture::lastTransferShieldedIdentifier, expected, sizeof(expected)) == 0);
}

// to_keys_json never carries an identifier in practice -> a random, non-zero one must be
// generated each call (regression guard against reintroducing the old hardcoded-zero identifier).
LOGOS_TEST(transfer_shielded_without_identifier_uses_random_nonzero_identifier) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string keysJson = std::string("{\"nullifier_public_key\":\"") + std::string(64, 'a') + "\"}";

    module.transfer_shielded(VALID_ID, keysJson, VALID_U128);
    uint8_t first[16];
    memcpy(first, MockWalletFfiCapture::lastTransferShieldedIdentifier, sizeof(first));

    module.transfer_shielded(VALID_ID, keysJson, VALID_U128);
    const uint8_t* second = MockWalletFfiCapture::lastTransferShieldedIdentifier;

    uint8_t zero[16] = {0};
    LOGOS_ASSERT_FALSE(memcmp(first, zero, sizeof(first)) == 0);
    LOGOS_ASSERT_FALSE(memcmp(first, second, sizeof(first)) == 0);
}

// transfer_private mirrors transfer_shielded's identifier handling exactly (see above).
LOGOS_TEST(transfer_private_with_identifier_forwards_it_unchanged) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string identifierHex(32, '7'); // 16 bytes of 0x77
    const std::string keysJson = std::string("{\"nullifier_public_key\":\"") + std::string(64, 'a')
        + "\",\"identifier\":\"" + identifierHex + "\"}";

    const nlohmann::json obj = parseObject(module.transfer_private(VALID_ID, keysJson, VALID_U128));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());

    uint8_t expected[16];
    memset(expected, 0x77, sizeof(expected));
    LOGOS_ASSERT(memcmp(MockWalletFfiCapture::lastTransferPrivateIdentifier, expected, sizeof(expected)) == 0);
}

LOGOS_TEST(transfer_private_without_identifier_uses_random_nonzero_identifier) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string keysJson = std::string("{\"nullifier_public_key\":\"") + std::string(64, 'a') + "\"}";

    module.transfer_private(VALID_ID, keysJson, VALID_U128);
    uint8_t first[16];
    memcpy(first, MockWalletFfiCapture::lastTransferPrivateIdentifier, sizeof(first));

    module.transfer_private(VALID_ID, keysJson, VALID_U128);
    const uint8_t* second = MockWalletFfiCapture::lastTransferPrivateIdentifier;

    uint8_t zero[16] = {0};
    LOGOS_ASSERT_FALSE(memcmp(first, zero, sizeof(first)) == 0);
    LOGOS_ASSERT_FALSE(memcmp(first, second, sizeof(first)) == 0);
}

LOGOS_TEST(register_public_account_invalid_hex_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.register_public_account("bad"));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_register_public_account"));
}

LOGOS_TEST(register_public_account_uses_convenience_path_outside_testnet) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.register_public_account(VALID_ID));

    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_register_public_account"));
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_send_generic_public_transaction"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
}

LOGOS_TEST(register_public_account_uses_deployed_testnet_program) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_get_sequencer_addr").returns("https://testnet.lez.logos.co");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.register_public_account(VALID_ID));

    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_send_generic_public_transaction"));
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_register_public_account"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicInstructionWords.size(), static_cast<size_t>(1));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicInstructionWords[0], static_cast<uint32_t>(1));
    const std::array<uint8_t, 32> expected_program_id = {
        0xdc, 0xbb, 0xfe, 0xbc, 0xd5, 0x93, 0x99, 0x96,
        0x1e, 0xd9, 0x97, 0x3b, 0x83, 0x07, 0xdc, 0x47,
        0x5f, 0xd4, 0xc5, 0xca, 0x57, 0x79, 0xaa, 0xcf,
        0xe7, 0x58, 0x8f, 0x7d, 0xbc, 0x3f, 0x4a, 0x71,
    };
    LOGOS_ASSERT(MockWalletFfiCapture::lastGenericPublicProgramId == expected_program_id);
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountIds.size(), static_cast<size_t>(1));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountKinds[0], static_cast<int>(FfiAccountIdentityKind::PUBLIC));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastGenericPublicAccountIds[0][0], static_cast<uint8_t>(0xaa));
}

LOGOS_TEST(generic_public_transaction_rejects_malformed_vectors) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json mismatched = parseObject(module.send_generic_public_transaction(
        {VALID_ID}, {}, {0}, std::string(64, 'c')
    ));
    LOGOS_ASSERT_FALSE(mismatched["success"].get<bool>());
    LOGOS_ASSERT_CONTAINS(mismatched["error"].get<std::string>(), std::string("same size"));
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_send_generic_public_transaction"));

    const nlohmann::json empty_accounts = parseObject(module.send_generic_public_transaction(
        {}, {}, {0}, std::string(64, 'c')
    ));
    LOGOS_ASSERT_FALSE(empty_accounts["success"].get<bool>());
    LOGOS_ASSERT_CONTAINS(empty_accounts["error"].get<std::string>(), std::string("account_ids"));

    const nlohmann::json empty_instruction = parseObject(module.send_generic_public_transaction(
        {VALID_ID}, {true}, {}, std::string(64, 'c')
    ));
    LOGOS_ASSERT_FALSE(empty_instruction["success"].get<bool>());
    LOGOS_ASSERT_CONTAINS(empty_instruction["error"].get<std::string>(), std::string("instruction"));
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_send_generic_public_transaction"));
}

LOGOS_TEST(register_private_account_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.register_private_account(VALID_ID));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_register_private_account"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
}

// ============================================================================
// Bridge (L1 Bedrock <-> L2)
// ============================================================================

LOGOS_TEST(bridge_withdraw_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.bridge_withdraw(VALID_ID, VALID_ID_2, 100));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_bridge_withdraw"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
    LOGOS_ASSERT_EQ(obj["tx_hash"].get<std::string>(), std::string("0xmocktxhash"));
    LOGOS_ASSERT_TRUE(obj["error"].get<std::string>().empty());
}

LOGOS_TEST(bridge_withdraw_invalid_hex_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.bridge_withdraw("bad", VALID_ID_2, 100));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_bridge_withdraw"));
}

LOGOS_TEST(bridge_withdraw_ffi_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_bridge_withdraw").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.bridge_withdraw(VALID_ID, VALID_ID_2, 100));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
}

// ============================================================================
// Vault claiming
// ============================================================================

LOGOS_TEST(get_vault_balance_invalid_hex_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.get_vault_balance("not-hex").empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_get_vault_balance"));
}

LOGOS_TEST(get_vault_balance_returns_decimal_string) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("get_vault_balance_value").returns(42);
    LEZCoreModule module;

    const std::string balance = module.get_vault_balance(VALID_ID);
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_get_vault_balance"));
    LOGOS_ASSERT_EQ(balance, std::string("42"));
}

LOGOS_TEST(get_vault_balance_ffi_error_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_get_vault_balance").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.get_vault_balance(VALID_ID).empty());
}

LOGOS_TEST(vault_claim_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim(VALID_ID, VALID_U128));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_vault_claim"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
    LOGOS_ASSERT_EQ(obj["tx_hash"].get<std::string>(), std::string("0xmocktxhash"));
    LOGOS_ASSERT_TRUE(obj["error"].get<std::string>().empty());
}

LOGOS_TEST(vault_claim_invalid_hex_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim("bad", VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_vault_claim"));
}

LOGOS_TEST(vault_claim_invalid_amount_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim(VALID_ID, "ff"));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_CONTAINS(obj["error"].get<std::string>(), std::string("amount"));
}

LOGOS_TEST(vault_claim_ffi_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_vault_claim").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim(VALID_ID, VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
}

LOGOS_TEST(vault_claim_private_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim_private(VALID_ID, VALID_U128));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_vault_claim_private"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
}

LOGOS_TEST(vault_claim_private_invalid_hex_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim_private("bad", VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_vault_claim_private"));
}

LOGOS_TEST(vault_claim_private_invalid_amount_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim_private(VALID_ID, "ff"));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_CONTAINS(obj["error"].get<std::string>(), std::string("amount"));
}

LOGOS_TEST(vault_claim_private_ffi_error_json) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_vault_claim_private").returns(static_cast<int>(INTERNAL_ERROR));
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.vault_claim_private(VALID_ID, VALID_U128));
    LOGOS_ASSERT_FALSE(obj["success"].get<bool>());
    LOGOS_ASSERT_FALSE(obj["error"].get<std::string>().empty());
}

// ============================================================================
// Pinata claiming
// ============================================================================

LOGOS_TEST(claim_pinata_invalid_hex_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.claim_pinata("bad", VALID_ID_2, VALID_U128).empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_claim_pinata"));
}

LOGOS_TEST(claim_pinata_invalid_solution_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(module.claim_pinata(VALID_ID, VALID_ID_2, "ab").empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_claim_pinata"));
}

LOGOS_TEST(claim_pinata_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const nlohmann::json obj = parseObject(module.claim_pinata(VALID_ID, VALID_ID_2, VALID_U128));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_claim_pinata"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
}

LOGOS_TEST(claim_pinata_already_initialized_invalid_siblings_returns_empty) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    // siblings json is not an array -> parse failure.
    const std::string result = module.claim_pinata_private_owned_already_initialized(
        VALID_ID, VALID_ID_2, VALID_U128, 0, "not-an-array");
    LOGOS_ASSERT_TRUE(result.empty());
    LOGOS_ASSERT_FALSE(t.cFunctionCalled("wallet_ffi_claim_pinata_private_owned_already_initialized"));
}

LOGOS_TEST(claim_pinata_already_initialized_success_json) {
    auto t = LogosTestContext("logos_execution_zone");
    LEZCoreModule module;

    const std::string siblings = std::string("[\"") + std::string(64, 'a') + std::string("\",\"") + std::string(64, 'b') + std::string("\"]");
    const nlohmann::json obj = parseObject(module.claim_pinata_private_owned_already_initialized(
        VALID_ID, VALID_ID_2, VALID_U128, 1, siblings));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_claim_pinata_private_owned_already_initialized"));
    LOGOS_ASSERT_TRUE(obj["success"].get<bool>());
}

// ============================================================================
// Wallet lifecycle
// ============================================================================

LOGOS_TEST(create_new_success_then_double_open_fails) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_create_new").returns(1); // non-null handle
    LEZCoreModule module;

    LOGOS_ASSERT_TRUE(!module.create_new("/cfg", "/store", "pw").empty());
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_create_new"));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastCreateStatisticsPath, std::string("/statistics.json"));
    // Second attempt: already open.
    LOGOS_ASSERT_EQ(module.create_new("/cfg", "/store", "pw"), "");
}

LOGOS_TEST(create_new_null_handle_returns_internal_error) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_create_new").returns(0); // null handle
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.create_new("/cfg", "/store", "pw"), "");
}

LOGOS_TEST(open_success) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_open").returns(1);
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.open("/cfg", "/store"), static_cast<int64_t>(SUCCESS));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_open"));
    LOGOS_ASSERT_EQ(MockWalletFfiCapture::lastOpenStatisticsPath, std::string("/statistics.json"));
}

LOGOS_TEST(save_forwards_return_code) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_save").returns(static_cast<int>(SUCCESS));
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.save(), static_cast<int64_t>(SUCCESS));
    LOGOS_ASSERT(t.cFunctionCalled("wallet_ffi_save"));
}

// ============================================================================
// Configuration
// ============================================================================

LOGOS_TEST(get_sequencer_addr_returns_string) {
    auto t = LogosTestContext("logos_execution_zone");
    t.mockCFunction("wallet_ffi_get_sequencer_addr").returns("10.0.0.1:9000");
    LEZCoreModule module;

    LOGOS_ASSERT_EQ(module.get_sequencer_addr(), std::string("10.0.0.1:9000"));
}
