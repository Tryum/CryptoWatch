#include "binance.h"

#include "binanceprivate.h"

Binance::Binance(QString key, QString secret, QObject* parent)
    : QObject(parent)
    , binance(new BinancePrivate(key, secret, this)) {
    
    QObject::connect(binance, &BinancePrivate::onAccounts, this, &Binance::onAccounts);
    QObject::connect(binance, &BinancePrivate::onCurrencies, this, &Binance::onCurrencies);
}

Binance::~Binance(){

}

void Binance::getCurrencies() const {
    binance->getCurrencies();
}

void Binance::getAccounts() const {
    binance->getAccounts();
}
