# The trade offer from deal_trade_offer.inc, then player 0 confirms first
# while player 1 reads at slow text speed: player 0 escrows and saves a few
# seconds before player 1 does. Ends with player 0's escrow on the flash
# and player 1 confirmed but not yet escrowed.
include deal_trade_offer.inc
1 text_speed 0
0 until_mem coop_deal.sDeal+13 1 900 A
1 until_mem coop_deal.sDeal+13 1 3000 A
0 until_mem coop_deal.sDeal 5 900
0 until_mem coop_deal.sSaver 0 600
1 expect coop_deal.sDeal 3
1 expect_txn 0
