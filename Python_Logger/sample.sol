// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract Simple {
    mapping(address => uint) balances;

    function deposit(uint amount) public {
        balances[msg.sender] += amount;
    }

    function transfer(address to, uint amount) public {
        require(balances[msg.sender] >= amount, "Insufficient");
        balances[msg.sender] -= amount;
        balances[to] += amount;
    }
}
