1. Motivation
Suppose you publish a Mynt wallet address for donation on your Twitter/etc profile page. Furthermore, you have an anonymous identity for some secret project where you also want to receive donation in Mynt. Because you don't want people to be able to associate you with that anonymous identity by Googling your address, you'd want to use separate wallet addresses for these two purposes. While you can easily do so by simply creating two wallets separately, you're going to spend twice the time for scanning the blockchain and need twice the storage for the wallet cache. And this cost grows in proportion to the number of additional wallet addresses you'd like to have.

Another relevant scenario is when you want to buy Mynt anonymously through an instant exchange like ShapeShift (note: you cannot buy Mynt from ShapeShift right now). If you used the same Mynt address repeatedly for multiple purchases, ShapeShift would know that the same person bought that much of Mynt through those purchases (although they don't know your contact info as they don't require account registration). You can anonymize the process by creating a temporary wallet for every purchase and transferring the fund to your real wallet afterwards. This is doable, but clearly tedious.

The scheme proposed in this PR provides an effective solution for such scenarios, where the recipient can create multiple wallet "subaddresses" that appear unrelated to each other to outside observers, and the recipient can recognize all incoming transfers to those subaddresses with almost no additional cost. Note that this scheme only introduces additional procedures for the wallet software to construct and interpret transactions, while not requiring any change in the consensus rule (thus requiring no hard fork).

2. How it works cryptographically
Suppose Bob's wallet address is (A, B) = (a*G, b*G) where a and b are his view and spend secret keys, respectively. Likewise, Alice's address is (X, Y) = (x*G, y*G). Bob is receiving Mynt from Alice, but he wants to do so without using his real address (A, B) in order to prevent Alice from knowing that the recipient is him. In the above example with ShapeShift, Bob would be you and Alice would be ShapeShift.

2.1. Generating a subaddress
Bob generates his i-th subaddress (i=1,2,...) as a pair of public keys (C,D) where:

m = Hs(a || i)
M = m*G
D = B + M
C = a*D

We call i the index of the subaddress. Note that (A,B) and (C,D) are unlinkable without the knowledge of the view secret key a. Bob then registers D to a hash table T stored in his wallet cache (akin to the aggregate addresses scheme):

T[D] <- i
To handle his main address in a unified way, Bob also registers B to the hash table:

T[B] <- 0
In other words, the index 0 is treated as a special case in this scheme representing the original standard address.

2.2. Sending to a subaddress
When Alice constructs a transaction that transfers some fund to a subaddress (C, D), she first chooses a random scalar s and generates a tx pubkey:

R = s*D
Note that the tx secret key r such that R = r*G is unknown to Alice because she doesn't know the secret key of D. She then computes an output pubkey for the destination:

P = Hs(s*C)*G + D
She finally computes an output pubkey to herself as her change:

Q = Hs(x*R)*G + Y
Importantly, without the knowledge of the tx secret key r as explained above, Alice can include her change output in the transaction only because she knows her own view secret key x.

Also note that Alice can prove her payment to Bob by using s.

2.3. Receiving by a subaddress
Bob checks if an output pubkey P in a new transaction belongs to him or not by computing

D' = P - Hs(a*R)*G
and looking for D' in the hash table. If the transaction was indeed bound to Bob's subaddress (C,D), D' should equal to D because

a*R = a*s*D
    = s*a*D
    = s*C
Therefore, Bob should be able to find D' in the hash table:

i <- T[D']
and obtain the private key of P:

p = { Hs(a*R) + b                   i == 0
    { Hs(a*R) + b + Hs(a || i)      otherwise
2.4. Grouping subaddresses
When sending funds to Bob's subaddress (C,D), Alice can choose to send the change to her i-th subaddress (X_i,Y_i) instead of her main address (X,Y) by replacing Y with Y_i when deriving the change's output pubkey:

Q = Hs(x*R)*G + Y_i
Using this scheme, it's now possible to maintain balances of subaddresses separately, making them virtually function as separate wallets. In order to maintain subaddresses in an organized manner, we propose a simple scheme of grouping subaddresses as follows: we define the index of subaddresses as a pair of indices (i,j) with i, the major index, representing a group of subaddresses (called an account) and j, the minor index, representing a particular subaddress within that account. Incoming transfers to subaddresses belonging to the same account (i,0), (i,1), ... are summed up to form a single balance, and any spending of those outputs will transfer the change to the base subaddress (i,0). The index (0,0) represents the original standard address.

3. How it works in the CLI
3.1. Synopsis
Existing commands with updated syntax:

address
address new <label text with white spaces allowed>
address all
address <index_min> [<index_max>]
address label <index> <label text with white spaces allowed>

balance [detail]

show_transfers [in|out|pending|failed|pool] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]
incoming_transfers [available|unavailable] [index=<N1>[,<N2>,...]]

transfer [index=<N1>[,<N2>,...]] [<priority>] [<mixin_count>] <address> <amount> [<payment_id>]
locked_transfer [index=<N1>[,<N2>,...]] [<priority>] [<mixin_count>] <addr> <amount> <lockblocks> [<payment_id>]

sweep_all [index=<N1>[,<N2>,...]] [<mixin_count>] <address> [<payment_id>]
New commands:

account
account new <label text with white spaces allowed>
account switch <index>
account label <index> <label text with white spaces allowed>
3.2. Managing accounts
Initially, the wallet has only one account corresponding to the major index 0. The account index is indicated as the number after / in the command prompt:

[wallet/0 XSo4qUM]: 
You can create a new account by using the command account new:

account new <label text with white spaces allowed>
[wallet/0 XSo4qUM]: account new Hosting Payments
           Account               Balance      Unlocked balance                 Label
       0 XSo4qUM           0.000000000           0.000000000       Primary account
       1 XSQVvGr           0.000000000           0.000000000      Hosting Payments
-------------------------------------------------------------------------------------
             Total           0.000000000           0.000000000

[wallet/1 SuboQVvGr]: address
0  SuboQVvGroM2HncSXDxGNk5ZFiAHZLokJDEoomwCUZ5Hj8fZDNXT2xuMyFYEu16quKLZVJaRDqqGCPTE5ouCTmSG3cPUj2Chii  Primary address
Note the change of the address. The command account switch lets you switch between accounts:

[wallet/1 SuboQVvGr]: account switch 0
Currently selected account: [0] Primary account
Balance: 0.000000000, unlocked balance: 0.000000000
[wallet/0 XSo4qUM]: address
0  XSo4qUMUm7SKBmDfDenhgQsixzFQG1TVrEi49HetA5XnrSQStnJNNXDQhcmoW4E2EKJNtzXqiLgbfgaYps2LYsDksmwViA89J  Primary address
The command account with no arguments shows the list of all the accounts along with their balances:

[wallet/0 XSo4qUM]: account
           Account               Balance      Unlocked balance                 Label
       0 XSo4qUM           0.000000000           0.000000000       Primary account
       1 XSQVvGr           0.000000000           0.000000000      Hosting Payments
-------------------------------------------------------------------------------------
             Total           0.000000000           0.000000000
At any time, the user operates on the currently selected account and specifies the minor index in various commands when necessary via the index parameter; i.e. in the following description, we use the term "index" to mean the minor index.

3.3. Generating subaddresses
The command address shows the "base" address at index=0 (which corresponds to the original standard address when the currently selected account is at index=0). The command address new lets you create a new address at one index beyond the currently existing highest index associated with this account. You can assign a label to the address if necessary:

address new <label text with white spaces allowed>
[wallet/1 XSQVvGr]: address new Payment from Alice
1  XSC2gkAthDkV2Q49UujeZUNs8TAPwHCddNdjoz2gdW3cTkBZypk8NjX823bwbXtW5TdAZ2x4PPeGm21qk7bViV5kmAtHU38A  Payment from Alice
Note that this command is intended to be used for generating "throwaway" addresses; i.e. when you just want a fresh new address to receive funds to the currently selected account (e.g. when receiving SUMO from a customer named Bob). If instead you want to maintain a separate balance associated with a new address, create a new account by the command account new.

The commands address all and address <index_min> [<index_max>] show lists of addresses that have been generated so far:

[wallet/1 XSQVvGr]: address all
0  XSQVvGroM2HncSXDxGNk5ZFiAHZLokJDEoomwCUZ5Hj8fZDNXT2xuMyFYEu16quKLZVJaRDqqGCPTE5ouCTmSG3cPUj2Chii  Primary address
1  XSC2gkAthDkV2Q49UujeZUNs8TAPwHCddNdjoz2gdW3cTkBZypk8NjX823bwbXtW5TdAZ2x4PPeGm21qk7bViV5kmAtHU38A  Payment from Alice
2  XSUXCgAH3Ap8oioetYvx4NSRPLtPcWFaxkMihVyP5qMTC4G3a3jxjJhnMNer52eBaBBuu4ssnERUnT8YMkf1B44CV4VeBdaB  Payment from Bob
3  XSQAbELLVSYrH557s9ev6fmCe85hhv9AMfqdvGrtAV4qd52mLHbFmJY7kfG3ryRd3oxaSRDTn22aUeYx9vCaev7B9mN6Fx2C  Payment from Carol
4  XSDmM6uBHEnZQ8oMLuHeEQsPcnyvcunimjfUQ6BxLj3BpJ6D9vaQn8AWseZ82YLWdVH1gZxkyJ59Zwt9uwoywT5SyS6A2Eui  Payment from Dave
5  XSTgh6ezdYozR1AwTmUfQ1g2sB3RFc2Z3TgaqnU96pKnGJzDBnNuyLcCvkgAceQnCfKDusoa83pD22sysGHYDa67QskSffL5  Payment from Eve
6  XSDfraQjfGQaAXMZ1xNU23irhaiaRzrL9KLB3itF1bdnp9KXdqDMt6Ze4ANYs64KDw79yE1Bb9b1SdF1gW4Zr88rFzn7CKYT  Payment from Frank

[wallet/1 XSQVvGr]: address 3 4
3  XSQAbELLVSYrH557s9ev6fmCe85hhv9AMfqdvGrtAV4qd52mLHbFmJY7kfG3ryRd3oxaSRDTn22aUeYx9vCaev7B9mN6Fx2C  Payment from Carol
4  XSDmM6uBHEnZQ8oMLuHeEQsPcnyvcunimjfUQ6BxLj3BpJ6D9vaQn8AWseZ82YLWdVH1gZxkyJ59Zwt9uwoywT5SyS6A2Eui  Payment from Dave
The command address label lets you set the label of an address:

address label <index> <label text with white spaces allowed>
[wallet/1 XSQVvGr]: address label 6 Eve paid
6  XSDfraQjfGQaAXMZ1xNU23irhaiaRzrL9KLB3itF1bdnp9KXdqDMt6Ze4ANYs64KDw79yE1Bb9b1SdF1gW4Zr88rFzn7CKYT  Eve paid
Note that the label of the base address is treated as the label of the currently selected account. You can change account labels by using either address label or account label:

[[wallet/0 XSo4qUM]: account label 0 Main Account
           Account               Balance      Unlocked balance                 Label
       0 XSo4qUM           0.000000000           0.000000000          Main Account
       1 XSQVvGr           0.000000000           0.000000000      Travel Payments
-------------------------------------------------------------------------------------
             Total           0.000000000           0.000000000
3.4. Checking funds and balances
Here the wallet finds a few incoming transfers by some of its subaddresses (indicated by the major-minor index pairs):

Height 55349, txid <a50c265a578722118defbf4e2fde6572984087e180650ac5b0edfef2d60741d1>, 1.000000000 SUMO, idx 1/1
Height 55357, txid <d0aa53a4fc12d66a341c5ef090a9c28f2d15a6e957eb02985d5368e34c8dcc37>, 1.100000000 SUMO, idx 1/2
Height 55358, txid <131aaafe18ffc204af5182898b5dfa021be8d12cf829360011d553fb7546fb9e>, 1.200000000 SUMO, idx 1/3
Height 55358, txid <1ba71029da97e85d082b30ef35968c4b4f6f4f194c6db839e4505092548df297>, 1.300000000 SUMO, idx 1/4

[wallet/1 SuboQVvGr]: account
           Account               Balance      Unlocked balance                 Label
       0 XSo4qUM           1.567890000           1.567890000          Main Account
       1 XSQVvGr           4.600000000           1.000000000      Hosting Payments
-------------------------------------------------------------------------------------
             Total           6.167890000           2.567890000
The command balance shows the total balance of all the funds received by all the addresses belonging to the currently selected account. You can see the details about how much funds are received by which address by providing an optional argument detail:

[wallet/0 XSo4qUM]: balance
Currently selected account: [0] Main Account
Balance: 1.567890000, unlocked balance: 1.567890000
[wallet/0 XSo4qUM]: balance detail
Currently selected account: [0] Main Account
Balance: 1.567890000, unlocked balance: 1.567890000
Balance per address:
           Address               Balance      Unlocked balance  Outputs                 Label
       0 XSo4qUM           1.567890000           1.567890000        1          Main Account

[wallet/0 XSo4qUM]: account switch 1
Currently selected account: [1] Hosting Payments
Balance: 4.600000000, unlocked balance: 4.600000000
[wallet/1 XSQVvGr]: balance
Currently selected account: [1] Hosting Payments
Balance: 4.600000000, unlocked balance: 4.600000000
[wallet/1 XSQVvGr]: balance detail
Currently selected account: [1] Hosting Payments
Balance: 4.600000000, unlocked balance: 4.600000000
Balance per address:
           Address               Balance      Unlocked balance  Outputs                 Label
       1 XSC2gkA           1.000000000           1.000000000        1    Payment from Alice
       2 XSUXCgA           1.100000000           1.100000000        1      Payment from Bob
       3 XSQAbEL           1.200000000           1.200000000        1    Payment from Carol
       4 XSDmM6u           1.300000000           1.300000000        1     Payment from Dave
Other commands for showing detailed information about funds have some changes of syntax:

show_transfers [in|out|pending|failed|pool] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]
incoming_transfers [available|unavailable] [index=<N1>[,<N2>,...]]
[wallet/1 XSQVvGr]: show_transfers index=1,3
   55349     in      04:28:58 AM          1.000000000 a50c265a578722118defbf4e2fde6572984087e180650ac5b0edfef2d60741d1 0000000000000000 -
   55358     in      04:47:25 AM          1.200000000 131aaafe18ffc204af5182898b5dfa021be8d12cf829360011d553fb7546fb9e 0000000000000000 -

[wallet/1 XSQVvGr]: incoming_transfers index=2,4
               amount   spent    unlocked  ringct    global index                                                               tx id      addr index
          1.100000000       F    unlocked  RingCT          186611  <d0aa53a4fc12d66a341c5ef090a9c28f2d15a6e957eb02985d5368e34c8dcc37>               2
          1.300000000       F    unlocked  RingCT          186617  <1ba71029da97e85d082b30ef35968c4b4f6f4f194c6db839e4505092548df297>               4
3.5. Transferring funds
The apparently unrelated subaddresses might get statistically linked if funds transferred to different subaddresses are used together as inputs in new transactions. For example, suppose Bob published two subaddresses (C1,D1) and (C2,D2) in different places, and Alice repeatedly transferred funds to these addresses not knowing that they both belong to Bob. If Bob repeatedly used outputs received by these subaddresses together as inputs in new transactions, Alice would notice multiple instances of transactions where each of the input ring signatures contains the output she created for the payments to these subaddresses. The probability of such transactions occurring by chance would be fairly low, so she can confidently guess that these subaddresses belong to the same person.

To prevent this kind of potential risk of linkability, the wallet tries its best to avoid using outputs belonging to different subaddresses together as inputs in a new transaction. If there exist any subaddresses with enough unlocked balance for the requested transfer, the wallet chooses one randomly. If there is no such subaddress, the wallet uses the minimal number of subaddresses to cover the required amount while showing a warning message. You can also explicitly tell the wallet from which subaddresses the outputs should be picked by using an optional argument index=<N1>[,<N2>,...]:

transfer [index=<N1>[,<N2>,...]] [<priority>] [<mixin_count>] <address> <amount> [<payment_id>]
[wallet/1 XSQVvGr]: balance detail
Currently selected account: [1] Hosting Payments
Balance: 4.600000000, unlocked balance: 4.600000000
Balance per address:
           Address               Balance      Unlocked balance  Outputs                 Label
       1 SuboC2gkA           1.000000000           1.000000000        1    Payment from Alice
       2 SuboUXCgA           1.100000000           1.100000000        1      Payment from Bob
       3 SuboQAbEL           1.200000000           1.200000000        1    Payment from Carol
       4 SuboDmM6u           1.300000000           1.300000000        1     Payment from Dave

[wallet/1 SuboQVvGr]: transfer XSo52J8KdSNepKDDn2NAFkiMkRXNkLu2ngQT8sd53kQ9qrjfqjD2SRJk9WoQUzioJMeuEzKAjt8C8Jvf6Ma1Y6RwaFTz9Cw9b 1
...
Transaction 1/1:
Spending from address index 3
Sending 1.000000000.  The transaction fee is 0.006631300.

[wallet/1 XSQVvGr]: transfer index=4 XSo52J8KdSNepKDDn2NAFkiMkRXNkLu2ngQT8sd53kQ9qrjfqjD2SRJk9WoQUzioJMeuEzKAjt8C8Jvf6Ma1Y6RwaFTz9Cw9b 1.2
...
Transaction 1/1:
Spending from address index 4
Sending 1.200000000.  The transaction fee is 0.006629800.

[wallet/1 XSQVvGr]: transfer XSo52J8KdSNepKDDn2NAFkiMkRXNkLu2ngQT8sd53kQ9qrjfqjD2SRJk9WoQUzioJMeuEzKAjt8C8Jvf6Ma1Y6RwaFTz9Cw9b 2
...
Transaction 1/1:
Spending from address index 3
Spending from address index 4
WARNING: Outputs of multiple addresses are being used together.
Sending 2.000000000.  The transaction fee is 0.007112300.
The command sweep_all has a similar syntax:

sweep_all [index=<N1>[,<N2>,...]] [<mixin_count>] <address> [<payment_id>]
If you omit the index parameter, one subaddress to be swept is chosen randomly (with index 0 being chosen last). You can use index=<N> to specify which subaddress to be swept. You can also either use index=<N1>,<N2>,... to sweep balances of multiple or all subaddresses.

[wallet/1 XSQVvGr]: sweep_all XSo52J8KdSNepKDDn2NAFkiMkRXNkLu2ngQT8sd53kQ9qrjfqjD2SRJk9WoQUzioJMeuEzKAjt8C8Jvf6Ma1Y6RwaFTz9Cw9b
...
Transaction 1/1:
Spending from address index 3

[wallet/1 XSQVvGr]: sweep_all index=1,4 XSo52J8KdSNepKDDn2NAFkiMkRXNkLu2ngQT8sd53kQ9qrjfqjD2SRJk9WoQUzioJMeuEzKAjt8C8Jvf6Ma1Y6RwaFTz9Cw9b
...
Transaction 1/1:
Spending from address index 1
Spending from address index 4
WARNING: Outputs of multiple addresses are being used together.
Sweeping 2.300000000 for a total fee of 0.007115700.  Is this okay?  (Y/Yes/N/No) N
Note that the change always goes to the base subaddress at index=0:

[wallet/1 XSQVvGr]: transfer index=4 XSo52J8KdSNepKDDn2NAFkiMkRXNkLu2ngQT8sd53kQ9qrjfqjD2SRJk9WoQUzioJMeuEzKAjt8C8Jvf6Ma1Y6RwaFTz9Cw9b 1
...
Transaction 1/1:
Spending from address index 4
Sending 1.000000000.  The transaction fee is 0.006631300.
Is this okay?  (Y/Yes/N/No): Y
Money successfully sent, transaction <968dff8cd863cb884b37bb946070ff455fdf72310d001fd0690901cdde69b8f0>
[wallet/1 XSQVvGr]: balance detail
Currently selected account: [1] Hosting Payments
Balance: 3.593368700, unlocked balance: 3.300000000
Balance per address:
           Address               Balance      Unlocked balance  Outputs                 Label
       0 XSQVvGr           0.293368700           0.000000000        0      Hosting Payments
       1 XSC2gkA           1.000000000           1.000000000        1    Payment from Alice
       2 XSUXCgA           1.100000000           1.100000000        1      Payment from Bob
       3 XSQAbEL           1.200000000           1.200000000        1    Payment from Carol
[wallet/1 XSQVvGr]:
3.6. Restoring wallet from seed
One slight caveat with this scheme is that when restoring a wallet from the seed, the wallet might miss transfers to subaddresses if they aren't stored in the hashtable yet. To mitigate this issue, for each account, the wallet stores 200 (a constant SUBADDRESS_LOOKAHEAD_MINOR defined in wallet2.h) subaddresses of indices beyond the highest index created so far. The wallet also generates 50 (a constant SUBADDRESS_LOOKAHEAD_MAJOR defined in wallet2.h) accounts beyond the highest index created so far. This means that the wallet restoration process is guaranteed to find incoming transfers to subaddresses as long as the major and minor indices of the used subaddresses differ by less than those predefined numbers. Note that the wallet expands the hashtable by itself as it finds incoming transfers to subaddresses at new higher indices, so normally the user won't need to worry about the management of the hashtable. Even if the differences of indices are bigger than those predifined numbers, you can still make the wallet recognize the incoming transfers by manually expanding the hashtable and rescanning the blockchain.

4. Credit
This is fork from Monero subaddress PR monero-project/monero#2056 by kenshi84 and other contributors.
