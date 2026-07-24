#ifndef MOCK_WALLET_FFI_CAPTURE_H
#define MOCK_WALLET_FFI_CAPTURE_H

// LogosCMockStore (logos_clib_mock.h) only lets tests control return values, not
// inspect call arguments. The transfer_shielded/transfer_private identifier logic
// needs the latter, so capture the relevant args here in plain static storage.

#include <array>
#include <cstdint>
#include <vector>

namespace MockWalletFfiCapture {

extern uint8_t lastTransferShieldedIdentifier[16];
extern uint8_t lastTransferPrivateIdentifier[16];
extern std::vector<uint32_t> lastGenericPublicInstructionWords;
extern std::array<uint8_t, 32> lastGenericPublicProgramId;
extern std::vector<std::array<uint8_t, 32>> lastGenericPublicAccountIds;
extern std::vector<int> lastGenericPublicAccountKinds;

} // namespace MockWalletFfiCapture

#endif // MOCK_WALLET_FFI_CAPTURE_H
