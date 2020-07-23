
#include "transaction/transaction.h"
#include "transaction/transaction_manager.h"

namespace pidan {

Transaction *TransactionManager::BeginTransaction() { return new Transaction(); }

}  // namespace pidan