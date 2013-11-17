#!/usr/bin/perl -w
use strict;

use Data::Dumper;

# open (OF, ">Orders.txt") || die;
exit &main();

my %buy_levels;
my %buy_orders;

my %sell_levels;
my %sell_orders;

my $id = 0;

#-----------------------------------------------------------------------------
sub getHashMin {
   my $href = shift;
   if (keys %{$href} == 0) {
      return 0;
   }
   return (sort keys %{$href})[0];
}

#-----------------------------------------------------------------------------
sub getHashMax {
   my $href = shift;
   if (keys %{$href} == 0) {
      return 0;
   }
   return (sort keys %{$href})[-1];
}

#-----------------------------------------------------------------------------
sub addBuy {
   my $side = 'B';
   my $offset = (int(rand(5)) + 1) / 100.;
   $offset = sprintf("%.2f", $offset);

   my $price = &getHashMax(\%buy_levels);
   if ($price == 0) { $price = 50.00; }
   $price += $offset;
   $price = sprintf("%.2f", $price);

   # cross it 1% of the time
   if (int(rand(100)) > 98) {
      $price = &getHashMin(\%sell_levels);
      if ($price == 0) { $price = 50.00; }

      $price += int(rand(5)) / 100.;
      $price = sprintf("%.2f", $price);
   }
   my $quantity = int(rand(100)) + 1;
   my $order_id = $id++;

   # .1% chance of the price being negative
   if (int(rand(1000)) == 999) {
      $price = $price * -1;
   }
   # only store in local map on a legit order
   else {
      $buy_orders{$order_id}->{price} = sprintf("%.2f", $price);
      $buy_orders{$order_id}->{quantity} = $quantity;
      $buy_orders{$order_id}->{order_id} = $order_id;

      push(@{$buy_levels{$price}}, $buy_orders{$order_id});
   }
   # keep losing precision on this thing.
   $price = sprintf("%.2f", $price);
   print "A,$order_id,$side,$quantity,$price\n";
}

#-----------------------------------------------------------------------------
sub addSell {
   my $side = 'S';
   my $offset = (int(rand(5)) + 1) / 100.;
   $offset = sprintf("%.2f", $offset);

   my $price = &getHashMin(\%sell_levels);
   if ($price == 0) { $price = 50.00; }
   $price += $offset;
   $price = sprintf("%.2f", $price);

   # cross it 1% of the time
   if (int(rand(100)) > 98) {
      $price = &getHashMax(\%buy_levels);
      if ($price == 0) { $price = 50.00; }

      $price -= int(rand(5)) / 100.;
      $price = sprintf("%.2f", $price);
   }
   my $quantity = int(rand(100)) + 1;
   my $order_id = $id++;

   # .1% chance of the price being negative
   if (int(rand(1000)) == 999) {
      $price = $price * -1;
   }
   # only store in local map on a legit order
   else {
      $sell_orders{$order_id}->{price} = sprintf("%.2f", $price);
      $sell_orders{$order_id}->{quantity} = $quantity;
      $sell_orders{$order_id}->{order_id} = $order_id;

      push(@{$sell_levels{$price}}, $sell_orders{$order_id});
   }
   # keep losing precision on this thing.
   $price = sprintf("%.2f", $price);
   print "A,$order_id,$side,$quantity,$price\n";
}

#------------------------------------------------------------------------------
# returns reference to order
sub getRandomOrder {
   my $ohr = shift;
   my @keys = sort keys %{$ohr};
   if (scalar(@keys) == 0) {
      return -1;
   }
   my $idx = int(rand(scalar(@keys)));
   return $ohr->{$keys[$idx]};
}

#------------------------------------------------------------------------------
sub getRandomOrderID {
   my $ohr = shift;
   my @keys = sort keys %{$ohr};
   if (scalar(@keys) == 0) {
      return -1;
   }
   return $keys[int(rand(scalar(@keys)))];
}

#------------------------------------------------------------------------------
sub checkIfOrderIDExists {
   my $order_id = shift;
   if (exists $buy_orders{$order_id} || exists $sell_orders{$order_id}) {
      return 1;
   }
   return 0;
}

#-----------------------------------------------------------------------------
sub checkIfPriceLevelExists {
   my $price_level = shift;
   if (exists $buy_levels{$price_level} || exists $buy_levels{$price_level}) {
      return 1;
   }
   return 0;
}

#------------------------------------------------------------------------------
sub removeOrderFromLevel {
   my ($order, $olr) = @_;

   my $index_to_remove = -1;
   for (my $i = 0; $i < scalar(@{$$olr{$order->{price}}}); ++$i) {
      if (@{$$olr{$order->{price}}}[$i]->{order_id} == $order->{order_id}) {
         $index_to_remove = $i;
         last;
      }
   }

   if ($index_to_remove == -1) {
      return 0;
   }

   splice @{$$olr{$order->{price}}}, $index_to_remove, 1;
   &clearLevelByPriceIfEmpty($order->{price}, $olr);
}

#------------------------------------------------------------------------------
sub clearLevelByPriceIfEmpty {
   my ($price, $olr) = @_;

   if (@{$olr->{$price}}) {
      return;
   }
   delete $olr->{$price};
}

#------------------------------------------------------------------------------
sub removeOrderFromHash {
   my ($order, $hr) = @_;

   if (exists($$hr{$order->{order_id}})) {
      delete $$hr{$order->{order_id}};
   }
}

#------------------------------------------------------------------------------
sub remove {
   my $order;
   my $level;

   my $side = int(rand(100));
   if ($side > 50) {
      if (keys(%buy_orders) == 0) { return; }
      $order = &getRandomOrder(\%buy_orders);
      &removeOrderFromHash($order, \%buy_orders);
      &removeOrderFromLevel($order, \%buy_levels);
      print "X,$order->{order_id},B,$order->{quantity},$order->{price}\n";
   }
   else {
      if (keys(%sell_orders) == 0) { return; }
      $order = &getRandomOrder(\%sell_orders);
      &removeOrderFromHash($order, \%sell_orders);
      &removeOrderFromLevel($order, \%sell_levels);
      print "X,$order->{order_id},S,$order->{quantity},$order->{price}\n";
   }
}

#------------------------------------------------------------------------------
sub modify {
   my $side = int(rand(100));
   if ($side > 50) {
      if (keys(%buy_orders) == 0) { return; }
      my $order_id = &getRandomOrderID(\%buy_orders);
      if ($order_id == -1) { return; }

      # increase vs. decrease qty
      my $new_qty = int(rand($buy_orders{$order_id}->{quantity} * 2)) + 1;

      if ($new_qty > $buy_orders{$order_id}->{quantity}) {
         # > quantity means change in priority
         &removeOrderFromLevel($buy_orders{$order_id}, \%buy_levels);
         $buy_orders{$order_id}->{quantity} = $new_qty;

         # perl is being stupid with floats.  Go perl.
         my $price = sprintf("%.2f", $buy_orders{$order_id}->{price});
         push(@{$buy_levels{$price}}, $buy_orders{$order_id});
      }
      # change price - 15% chance
      elsif (int(rand(100)) > 84) {
         &removeOrderFromLevel($buy_orders{$order_id}, \%buy_levels);

         my $price;
         if (int (rand(100)) > 50) {
            $price = $buy_orders{$order_id}->{price} += (int(rand(5)) + 1) / 100.;
         }
         else {
            $price = $buy_orders{$order_id}->{price} -= (int(rand(5)) + 1) / 100.;
         }
         $price = sprintf("%.2f", $price);
         if ($price <= 0) {
            return;
         }
         $buy_orders{$order_id}->{price} = $price;
         push(@{$buy_levels{$price}}, $buy_orders{$order_id});
      }
      # change qty down, no loss in priority
      else {
         $buy_orders{$order_id}->{quantity} = $new_qty;
      }

      print "M,$order_id,B,$buy_orders{$order_id}->{quantity},$buy_orders{$order_id}->{price}\n";
   }
   else {
      if (keys(%sell_orders) == 0) { return; }
      my $order_id = &getRandomOrderID(\%sell_orders);
      if ($order_id == -1) { return; }

      # increase vs. decrease qty
      my $new_qty = int(rand($sell_orders{$order_id}->{quantity} * 2)) + 1;

      if ($new_qty > $sell_orders{$order_id}->{quantity}) {
         # > quantity means change in priority
         &removeOrderFromLevel($sell_orders{$order_id}, \%sell_levels);
         $sell_orders{$order_id}->{quantity} = $new_qty;

         # perl is being stupid with floats.  Go perl.
         my $price = sprintf("%.2f", $sell_orders{$order_id}->{price});
         push(@{$sell_levels{$price}}, $sell_orders{$order_id});
      }
      # change price - 15% chance
      elsif (int(rand(100)) > 84) {
         &removeOrderFromLevel($sell_orders{$order_id}, \%sell_levels);

         my $price;
         if (int (rand(100)) > 50) {
            $price = $sell_orders{$order_id}->{price} += (int(rand(5)) + 1) / 100.;
         }
         else {
            $price = $sell_orders{$order_id}->{price} -= (int(rand(5)) + 1) / 100.;
         }
         $price = sprintf("%.2f", $price);
         if ($price <= 0) {
            return;
         }
         $sell_orders{$order_id}->{price} = $price;
         push(@{$sell_levels{$price}}, $sell_orders{$order_id});
      }
      # change qty down, no loss in priority
      else {
         $sell_orders{$order_id}->{quantity} = $new_qty;
      }

      print "M,$order_id,S,$sell_orders{$order_id}->{quantity},$sell_orders{$order_id}->{price}\n";
   }
}

#------------------------------------------------------------------------------
sub trade {
   # Pair order to order, 1 from buy max, 1 from sell min
   my $sell_min = &getHashMin(\%sell_levels);
   my $buy_max = &getHashMax(\%buy_levels);
   if ($sell_min == 0 || $buy_max == 0) {
      return;
   }
   if ($sell_min > $buy_max) {
      return;
   }

   my $sell_order = $sell_levels{$sell_min}[0];
   my $buy_order = $buy_levels{$buy_max}[0];

   my $trade_price = $sell_min;
   my $trade_quantity = 0;
   if ($sell_order->{quantity} <= $buy_order->{quantity}) {
      $trade_quantity = $sell_order->{quantity};
   }
   else {
      $trade_quantity = $buy_order->{quantity};
   }

   print "T,$trade_quantity,$trade_price\n";

   # delete buy and/or sell order if they're depleted on book
   $sell_order->{quantity} -= $trade_quantity;
   $buy_order->{quantity} -= $trade_quantity;

   if ($sell_order->{quantity} == 0) {
      &removeOrderFromHash($sell_order, \%sell_orders);
      &removeOrderFromLevel($sell_order, \%sell_levels);
   }
   if ($buy_order->{quantity} == 0) {
      &removeOrderFromHash($buy_order, \%buy_orders);
      &removeOrderFromLevel($buy_order, \%buy_levels);
   }
}

#------------------------------------------------------------------------------
sub duplicateAdd {
   my $side = "";
   my $order_id = -1;
   if (int(rand(100)) > 50) {
      $side = "B";
      $order_id = &getRandomOrderID(\%buy_orders);
      if ($order_id == -1) { return; }
   }
   else {
      $side = "S";
      $order_id = &getRandomOrderID(\%sell_orders);
      if ($order_id == -1) { return; }
   }
   my $quantity = int(rand(100)) + 1;
   my $price = int(rand(1000)) / 1000. + 1;
   print "A,$order_id,$side,$quantity,$price\n";
}

#------------------------------------------------------------------------------
sub badQuantity {
   my $side = int(rand(100)) > 50 ? 'B' : 'S';
   my $order_id = $id++;
   my $price = 1234;

   my $quantity;
   # negative #
   if (int(rand(100)) > 50) { 
      $quantity = int(rand(1000));
      $quantity *= -1;
   }
   # garbage value
   else {
      my @chars = ("A".."Z", "a".."z");
      $quantity .= $chars[rand @chars] for 1..10;
   }
   print "A,$order_id,$side,$quantity,$price\n";
}

#------------------------------------------------------------------------------
sub invalidID {
   my $side = int(rand(100)) > 50 ? 'B' : 'S';
   my $price = 1234;
   my $quantity = 10;

   my $order_id;
   # negative #
   if (int(rand(100)) > 50) { 
      $order_id = int(rand(1000)) + 1;
      $order_id *= -1;
   }
   # garbage value
   else {
      my @chars = ("A".."Z", "a".."z");
      $order_id .= $chars[rand @chars] for 1..10;
   }
   print "A,$order_id,$side,$quantity,$price\n";
}

#------------------------------------------------------------------------------
sub missingModify {
   my $order_id = 0;
   # could infinite loop here if you decide to create a *lot* of orders.
   while (&checkIfOrderIDExists($order_id) == 1) {
      $order_id = int(rand(1000000));
   }
   my $side = int(rand(100)) > 50 ? 'B' : 'S';
   my $quantity = (int(rand(10000)) + 1) / 100.;
   my $price = int(rand(1000)) + 1;
   $price = sprintf("%.2f", $price);
   print "M,$order_id,$side,$quantity,$price\n";
}

#------------------------------------------------------------------------------
sub cancelMissing {
   my $order_id = int(rand(10000));
   while (&checkIfOrderIDExists($order_id) == 1) {
      $order_id = int(rand(1000000));
   }
   my $side = int(rand(100)) > 50 ? "B" : "S";
   my $quantity = int(rand(1000)) + 1;
   my $price = (int(rand(10000)) + 1) / 100.;
   $price = sprintf("%.2f", $price);
   print "X,$order_id,$side,$quantity,$price\n";
}

#------------------------------------------------------------------------------
sub tradeMissing {
   my $price_level = int(rand(10000)) + 1 / 100.;
   while (&checkIfPriceLevelExists($price_level) == 1) {
      $price_level = int(rand(10000)) + 1 / 100.;
   }
   my $quantity = int(rand(1000)) + 1;
   print "T,$quantity,$price_level\n"
}

#------------------------------------------------------------------------------
sub corrupt {
   my @chars = ("A".."Z", "a".."z");
   my $buffer;
   $buffer .= $chars[rand @chars] for 1..1000;
   print "$buffer\n";
}

#-----------------------------------------------------------------------------
sub printBook {
   print "------------------------------\n";
   foreach my $key (reverse sort keys(%sell_levels)) {
      printf("%.2f ", $key);
      foreach my $order (@{$sell_levels{$key}}) {
         print "S " . $$order{quantity} . " ";
      }
      print "\n";
   }
   print "\n";
   foreach my $key (reverse sort keys(%buy_levels)) {
      printf("%.2f ", $key);
      foreach my $order (@{$buy_levels{$key}}) {
         print "B " . $$order{quantity} . " ";
      }
      print "\n";
   }
}

#------------------------------------------------------------------------------
sub main {
   for (my $i = 0; $i < 25000; ++$i) {
      # &printBook();

      # 1% chance of any of the bad things happening
      if (int(rand(100)) == 99) { &duplicateAdd(); }
      if (int(rand(100)) == 99) { &badQuantity(); }
      if (int(rand(100)) == 99) { &invalidID(); }
      if (int(rand(100)) == 99) { &missingModify(); }
      if (int(rand(100)) == 99) { &cancelMissing(); }
      if (int(rand(100)) == 99) { &tradeMissing(); }
      if (int(rand(100)) == 99) { &corrupt(); }

      if (keys(%buy_levels) > 0 && keys(%sell_levels) > 0) {
         # unwind all crossed orders
         while (&getHashMax(\%buy_levels) >= &getHashMin(\%sell_levels)) {
            # 1% chance to leave book crossed
            if (int(rand(100)) == 99) {
               # print "LEAVING CROSSED BOOK\n";
               last;
            }
            &trade();
            # &printBook();
         }
      }

      my $type = int(rand(100));
      if ($type <= 15) {
         &modify();
      } elsif ($type <= 30) {
         &remove();
      } else {
         int(rand(100)) > 50 ? &addBuy() : &addSell(); # 55% add
      }
   }
}
