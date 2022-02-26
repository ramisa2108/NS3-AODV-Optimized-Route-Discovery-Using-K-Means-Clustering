/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on
 *      NS-2 aodvKmeans model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      aodvKmeans-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/aodvKmeans-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "aodvKmeans-rtable.h"
#include <algorithm>
#include <iomanip>
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("aodvKmeansRoutingTable");

namespace aodvKmeans {

/*
 The Routing Table
 */

RoutingTableEntry::RoutingTableEntry (Ptr<NetDevice> dev, Ipv4Address dst, bool vSeqNo, uint32_t seqNo,
                                      Ipv4InterfaceAddress iface, uint16_t hops, Ipv4Address nextHop, Time lifetime,
                                      uint32_t txError, uint32_t positionX, uint32_t positionY, uint32_t freeSpace)
  : m_ackTimer (Timer::CANCEL_ON_DESTROY),
    m_validSeqNo (vSeqNo),
    m_seqNo (seqNo),
    m_hops (hops),
    m_lifeTime (lifetime + Simulator::Now ()),
    m_iface (iface),
    m_flag (VALID),
    m_reqCount (0),
    m_blackListState (false),
    m_blackListTimeout (Simulator::Now ()),
    m_txerrorCount(txError),
    m_positionX(positionX),
    m_positionY(positionY),
    m_freeSpace(freeSpace)
{
  m_ipv4Route = Create<Ipv4Route> ();
  m_ipv4Route->SetDestination (dst);
  m_ipv4Route->SetGateway (nextHop);
  m_ipv4Route->SetSource (m_iface.GetLocal ());
  m_ipv4Route->SetOutputDevice (dev);
}

RoutingTableEntry::~RoutingTableEntry ()
{
}

bool
RoutingTableEntry::InsertPrecursor (Ipv4Address id)
{
  NS_LOG_FUNCTION (this << id);
  if (!LookupPrecursor (id))
    {
      m_precursorList.push_back (id);
      return true;
    }
  else
    {
      return false;
    }
}

bool
RoutingTableEntry::LookupPrecursor (Ipv4Address id)
{
  NS_LOG_FUNCTION (this << id);
  for (std::vector<Ipv4Address>::const_iterator i = m_precursorList.begin (); i
       != m_precursorList.end (); ++i)
    {
      if (*i == id)
        {
          NS_LOG_LOGIC ("Precursor " << id << " found");
          return true;
        }
    }
  NS_LOG_LOGIC ("Precursor " << id << " not found");
  return false;
}

bool
RoutingTableEntry::DeletePrecursor (Ipv4Address id)
{
  NS_LOG_FUNCTION (this << id);
  std::vector<Ipv4Address>::iterator i = std::remove (m_precursorList.begin (),
                                                      m_precursorList.end (), id);
  if (i == m_precursorList.end ())
    {
      NS_LOG_LOGIC ("Precursor " << id << " not found");
      return false;
    }
  else
    {
      NS_LOG_LOGIC ("Precursor " << id << " found");
      m_precursorList.erase (i, m_precursorList.end ());
    }
  return true;
}

void
RoutingTableEntry::DeleteAllPrecursors ()
{
  NS_LOG_FUNCTION (this);
  m_precursorList.clear ();
}

bool
RoutingTableEntry::IsPrecursorListEmpty () const
{
  return m_precursorList.empty ();
}

void
RoutingTableEntry::GetPrecursors (std::vector<Ipv4Address> & prec) const
{
  NS_LOG_FUNCTION (this);
  if (IsPrecursorListEmpty ())
    {
      return;
    }
  for (std::vector<Ipv4Address>::const_iterator i = m_precursorList.begin (); i
       != m_precursorList.end (); ++i)
    {
      bool result = true;
      for (std::vector<Ipv4Address>::const_iterator j = prec.begin (); j
           != prec.end (); ++j)
        {
          if (*j == *i)
            {
              result = false;
            }
        }
      if (result)
        {
          prec.push_back (*i);
        }
    }
}

void
RoutingTableEntry::Invalidate (Time badLinkLifetime)
{
  NS_LOG_FUNCTION (this << badLinkLifetime.As (Time::S));
  if (m_flag == INVALID)
    {
      return;
    }
  m_flag = INVALID;
  m_reqCount = 0;
  m_lifeTime = badLinkLifetime + Simulator::Now ();
}

void
RoutingTableEntry::Print (Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
  std::ostream* os = stream->GetStream ();
  // Copy the current ostream state
  std::ios oldState (nullptr);
  oldState.copyfmt (*os);

  *os << std::resetiosflags (std::ios::adjustfield) << std::setiosflags (std::ios::left);

  std::ostringstream dest, gw, iface, expire;
  dest << m_ipv4Route->GetDestination ();
  gw << m_ipv4Route->GetGateway ();
  iface << m_iface.GetLocal ();
  expire << std::setprecision (2) << (m_lifeTime - Simulator::Now ()).As (unit);
  *os << std::setw (16) << dest.str();
  *os << std::setw (16) << gw.str();
  *os << std::setw (16) << iface.str();
  *os << std::setw (16);
  switch (m_flag)
    {
    case VALID:
      {
        *os << "UP";
        break;
      }
    case INVALID:
      {
        *os << "DOWN";
        break;
      }
    case IN_SEARCH:
      {
        *os << "IN_SEARCH";
        break;
      }
    }

  *os << std::setw (16) << expire.str();
  *os << m_hops << std::endl;
  // Restore the previous ostream state
  (*os).copyfmt (oldState);
}

/*
 The Routing Table
 */

RoutingTable::RoutingTable (Time t)
  : m_badLinkLifetime (t)
{
}

bool
RoutingTable::LookupRoute (Ipv4Address id, RoutingTableEntry & rt)
{
  NS_LOG_FUNCTION (this << id);
  Purge ();
  if (m_ipv4AddressEntry.empty ())
    {
      NS_LOG_LOGIC ("Route to " << id << " not found; m_ipv4AddressEntry is empty");
      return false;
    }
  std::map<Ipv4Address, RoutingTableEntry>::const_iterator i =
    m_ipv4AddressEntry.find (id);
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Route to " << id << " not found");
      return false;
    }
  rt = i->second;
  NS_LOG_LOGIC ("Route to " << id << " found");
  return true;
}

bool
RoutingTable::LookupValidRoute (Ipv4Address id, RoutingTableEntry & rt)
{
  NS_LOG_FUNCTION (this << id);
  if (!LookupRoute (id, rt))
    {
      NS_LOG_LOGIC ("Route to " << id << " not found");
      return false;
    }
  NS_LOG_LOGIC ("Route to " << id << " flag is " << ((rt.GetFlag () == VALID) ? "valid" : "not valid"));
  return (rt.GetFlag () == VALID);
}

bool
RoutingTable::DeleteRoute (Ipv4Address dst)
{
  NS_LOG_FUNCTION (this << dst);
  Purge ();
  if (m_ipv4AddressEntry.erase (dst) != 0)
    {
      NS_LOG_LOGIC ("Route deletion to " << dst << " successful");
      return true;
    }
  NS_LOG_LOGIC ("Route deletion to " << dst << " not successful");
  return false;
}

bool
RoutingTable::AddRoute (RoutingTableEntry & rt)
{
  NS_LOG_FUNCTION (this);
  Purge ();
  if (rt.GetFlag () != IN_SEARCH)
    {
      rt.SetRreqCnt (0);
    }
  std::pair<std::map<Ipv4Address, RoutingTableEntry>::iterator, bool> result =
    m_ipv4AddressEntry.insert (std::make_pair (rt.GetDestination (), rt));
  return result.second;
}

bool
RoutingTable::Update (RoutingTableEntry & rt)
{
  NS_LOG_FUNCTION (this);
  std::map<Ipv4Address, RoutingTableEntry>::iterator i =
    m_ipv4AddressEntry.find (rt.GetDestination ());
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Route update to " << rt.GetDestination () << " fails; not found");
      return false;
    }
  i->second = rt;
  if (i->second.GetFlag () != IN_SEARCH)
    {
      NS_LOG_LOGIC ("Route update to " << rt.GetDestination () << " set RreqCnt to 0");
      i->second.SetRreqCnt (0);
    }
  return true;
}

bool
RoutingTable::SetEntryState (Ipv4Address id, RouteFlags state)
{
  NS_LOG_FUNCTION (this);
  std::map<Ipv4Address, RoutingTableEntry>::iterator i =
    m_ipv4AddressEntry.find (id);
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Route set entry state to " << id << " fails; not found");
      return false;
    }
  i->second.SetFlag (state);
  i->second.SetRreqCnt (0);
  NS_LOG_LOGIC ("Route set entry state to " << id << ": new state is " << state);
  return true;
}

void
RoutingTable::GetListOfDestinationWithNextHop (Ipv4Address nextHop, std::map<Ipv4Address, uint32_t> & unreachable )
{
  NS_LOG_FUNCTION (this);
  Purge ();
  unreachable.clear ();
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i =
         m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); ++i)
    {
      if (i->second.GetNextHop () == nextHop)
        {
          NS_LOG_LOGIC ("Unreachable insert " << i->first << " " << i->second.GetSeqNo ());
          unreachable.insert (std::make_pair (i->first, i->second.GetSeqNo ()));
        }
    }
}

void
RoutingTable::InvalidateRoutesWithDst (const std::map<Ipv4Address, uint32_t> & unreachable)
{
  NS_LOG_FUNCTION (this);
  Purge ();
  for (std::map<Ipv4Address, RoutingTableEntry>::iterator i =
         m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); ++i)
    {
      for (std::map<Ipv4Address, uint32_t>::const_iterator j =
             unreachable.begin (); j != unreachable.end (); ++j)
        {
          if ((i->first == j->first) && (i->second.GetFlag () == VALID))
            {
              NS_LOG_LOGIC ("Invalidate route with destination address " << i->first);
              i->second.Invalidate (m_badLinkLifetime);
            }
        }
    }
}

void
RoutingTable::DeleteAllRoutesFromInterface (Ipv4InterfaceAddress iface)
{
  NS_LOG_FUNCTION (this);
  if (m_ipv4AddressEntry.empty ())
    {
      return;
    }
  for (std::map<Ipv4Address, RoutingTableEntry>::iterator i =
         m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); )
    {
      if (i->second.GetInterface () == iface)
        {
          std::map<Ipv4Address, RoutingTableEntry>::iterator tmp = i;
          ++i;
          m_ipv4AddressEntry.erase (tmp);
        }
      else
        {
          ++i;
        }
    }
}
std::vector<Ipv4Address> 
RoutingTable::Kmeans (Ipv4Address dst, uint32_t positionX, uint32_t positionY)
{
    Purge();
   // std::cout << "For sending to " << dst << "\n";
  
    /// features list
    int n = m_ipv4AddressEntry.size();
    double features[n][3];
    int k = 2;
    double cluster_center[k][3];
    int cluster_assignments[n];

  
    int i = 0;
    
    for (std::map<Ipv4Address, RoutingTableEntry>::iterator it = m_ipv4AddressEntry.begin (); it != m_ipv4AddressEntry.end (); ++it)
    {
      if(it->first.IsBroadcast() || it->first.IsLocalhost() || it->first.IsMulticast() || 
      it->first.IsSubnetDirectedBroadcast(Ipv4Mask((char *)"255.255.255.0")) || it->second.GetFlag() == INVALID || it->second.GetHop() > 2)
      {
        continue;
      }
      features[i][0] = 1.0 * (positionX - it->second.GetPositionX()) * (positionX - it->second.GetPositionX()) + 
                      1.0 * (positionY - it->second.GetPositionY()) * (positionY - it->second.GetPositionY());
                    
      features[i][1] = 1.0 * it->second.GetTxErrorCount();
      features[i][2] = 1.0 * it->second.GetFreeSpace();
      i++;
    }
    
    n = i;

    double mini[3], maxi[3];

    // normalize the features
    for(int j=0;j<3;j++)
    {
      mini[j] = features[0][j];
      maxi[j] = features[0][j];
      for(int i=1;i<n;i++)
      {
        mini[j] = std::min(mini[j], features[i][j]);
        maxi[j]= std::max(maxi[j], features[i][j]);
      }

      for(int i=0;i<n;i++)
      {
        features[i][j] = (features[i][j] - mini[j]);
        if(mini[j] != maxi[j]) features[i][j]/= (maxi[j] - mini[j]);
      }

    }
    

    // define an idea feature list
    double ideal[3];
    ideal[0] = 0.0; // minimum possible distance
    ideal[1] = 0.0; // minimum possible error
    ideal[2] = maxi[2]; // buffer empty
    

    // randomly pick k cluster heads
    srand(time(0));
    for(int i=0;i<k;i++)
    {
      int p = rand() % n;
      for(int j=0;j<3;j++)
      {
        cluster_center[i][j] = features[p][j];
      }
    }

    int num_iterations = 3;
    // run iterations
    for(int iteration=0;iteration<=num_iterations;iteration++)
    {
      
      // assign clusters
      
      // std::cout << cluster_center[0][0] << " " << cluster_center[0][1] << " " << cluster_center[0][2] << " " << cluster_center[0][3] << "\n";
      // std::cout << cluster_center[1][0] << " " << cluster_center[1][1] << " " << cluster_center[1][2] << " " << cluster_center[1][3] << "\n";

      for(int i=0;i<n;i++)
      {
          double mini_dist = -1.0;
          for(int j=0;j<k;j++)
          {
            double dist = 0.0;
            for(int d=0; d<3; d++)
            {
              dist += (features[i][d] - cluster_center[j][d]) * (features[i][d] - cluster_center[j][d]);
               
            }
            if(mini_dist == -1.0 || dist < mini_dist)
            {
              mini_dist = dist;
              cluster_assignments[i] = j;
            }
          }
          
      }
      // if(iteration == 0)
      // {
        // std::cout << "Initial assignment\n";
        // for(int i=0;i<n;i++)
        // {
        //   std::cout << m_nb[i].m_neighborAddress << " " << cluster_assignments[i] << "\n";
        // }
      // }

      if(iteration == num_iterations) 
      {
        break;
      }

      // re-position cluster centers
      
      for(int j=0;j<k;j++)
      {
        int cnt = 0;
        for(int d=0;d<3;d++)
        {
          cluster_center[j][d] = 0.0;
        }


        for(int i=0;i<n;i++)
        {
          if(cluster_assignments[i] == j)
          {
            for(int d=0;d<3;d++)
            {
              cluster_center[j][d] += features[i][d];
              
            }
            
            cnt++;
          }
          
        }
        if(cnt)
        {
          for(int d=0;d<3;d++)
          {
            cluster_center[j][d] /= cnt;
          }
        }
        else 
        {
          int p = rand() % n;
          for(int d=0;d<3;d++)
          {
            cluster_center[j][d] = features[p][d];
          }
        }

        
      }
      

    }

    // std::cout << "End results:\n";
    // for(int i=0;i<n;i++)
    // {
    //   std::cout << m_nb[i].m_neighborAddress << ": " << cluster_assignments[i] << "\n";
    // }

    // de-normalize cluster centers
    
    //std::cout << "after denormalizing:\n";
    for(int j=0;j<k;j++)
    {
      for(int d=0;d<3;d++)
      {
        cluster_center[j][d] *= (maxi[d] - mini[d]);
        cluster_center[j][d] += mini[d];
        //std::cout << cluster_center[j][d] << ' ';
      }
      //std::cout << '\n';
    }
    for(int i=0;i<n;i++)
    {
      //std::cout << m_nb[i].m_neighborAddress << ": ";
      for(int d=0;d<3;d++)
      {
        features[i][d] *= (maxi[d] - mini[d]);
        features[i][d] += mini[d];
        //std::cout << features[i][d] << ' ';
      }
      //std::cout << '\n';
    }

    // select optimal cluster
    int optimal_cluster = -1;
    double mini_dist = -1.0;
    for(int j=0;j<k;j++)
    {
      double dist = 0;
      for(int d=0; d<3; d++)
      {
          dist += (cluster_center[j][d] - ideal[d]) * (cluster_center[j][d] - ideal[d]); 
      }
      if(mini_dist == -1.0 || dist < mini_dist)
      {
        mini_dist = dist;
        optimal_cluster = j;
      }

    }

    std::vector<Ipv4Address>selectedCluster;
    i=0;
    //std::cout << "Selected cluster for:" << dst << "\n";
    for (std::map<Ipv4Address, RoutingTableEntry>::iterator it = m_ipv4AddressEntry.begin (); it != m_ipv4AddressEntry.end (); ++it)
    {
      if(it->first.IsBroadcast() || it->first.IsLocalhost() || it->first.IsMulticast() || 
      it->first.IsSubnetDirectedBroadcast(Ipv4Mask((char *)"255.255.255.0")) || it->second.GetFlag() == INVALID || it->second.GetHop() > 2)
      {
        continue;
      }
      if(cluster_assignments[i] == optimal_cluster)
      {
        selectedCluster.push_back(it->first);
        //std::cout << it->first <<" ";
      }
      i++;
    }
    //std::cout << "\n";
    
    return selectedCluster;

    // std::cout << "optimal cluster = " << optimal_cluster << "\n";


    // select cluster head 
    
    // int optimal_node = -1;
    // mini_dist = -1.0;
    // for(int i=0;i<n;i++)
    // {
    //   if(cluster_assignments[i] == optimal_cluster)
    //   {
    //     double dist = 0;
    //     for(int d=0; d<4; d++)
    //     {
    //         dist += (features[i][d] - ideal[d]) * (features[i][d] - ideal[d]); 
    //     }
    //     if(mini_dist == -1.0 || dist < mini_dist)
    //     {
    //       mini_dist = dist;
    //       optimal_node = i;
    //     }
    //   }

    // }

    
    // Ipv4Address selectedAddress = m_nb[optimal_node].m_neighborAddress;

    // return selectedAddress;
}


void
RoutingTable::Purge ()
{
  NS_LOG_FUNCTION (this);
  if (m_ipv4AddressEntry.empty ())
    {
      return;
    }
  for (std::map<Ipv4Address, RoutingTableEntry>::iterator i =
         m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); )
    {
      if (i->second.GetLifeTime () < Seconds (0))
        {
          if (i->second.GetFlag () == INVALID)
            {
              std::map<Ipv4Address, RoutingTableEntry>::iterator tmp = i;
              ++i;
              m_ipv4AddressEntry.erase (tmp);
            }
          else if (i->second.GetFlag () == VALID)
            {
              NS_LOG_LOGIC ("Invalidate route with destination address " << i->first);
              i->second.Invalidate (m_badLinkLifetime);
              ++i;
            }
          else
            {
              ++i;
            }
        }
      else
        {
          ++i;
        }
    }
}

void
RoutingTable::Purge (std::map<Ipv4Address, RoutingTableEntry> &table) const
{
  NS_LOG_FUNCTION (this);
  if (table.empty ())
    {
      return;
    }
  for (std::map<Ipv4Address, RoutingTableEntry>::iterator i =
         table.begin (); i != table.end (); )
    {
      if (i->second.GetLifeTime () < Seconds (0))
        {
          if (i->second.GetFlag () == INVALID)
            {
              std::map<Ipv4Address, RoutingTableEntry>::iterator tmp = i;
              ++i;
              table.erase (tmp);
            }
          else if (i->second.GetFlag () == VALID)
            {
              NS_LOG_LOGIC ("Invalidate route with destination address " << i->first);
              i->second.Invalidate (m_badLinkLifetime);
              ++i;
            }
          else
            {
              ++i;
            }
        }
      else
        {
          ++i;
        }
    }
}

bool
RoutingTable::MarkLinkAsUnidirectional (Ipv4Address neighbor, Time blacklistTimeout)
{
  NS_LOG_FUNCTION (this << neighbor << blacklistTimeout.As (Time::S));
  std::map<Ipv4Address, RoutingTableEntry>::iterator i =
    m_ipv4AddressEntry.find (neighbor);
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Mark link unidirectional to  " << neighbor << " fails; not found");
      return false;
    }
  i->second.SetUnidirectional (true);
  i->second.SetBlacklistTimeout (blacklistTimeout);
  i->second.SetRreqCnt (0);
  NS_LOG_LOGIC ("Set link to " << neighbor << " to unidirectional");
  return true;
}

void
RoutingTable::Print (Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
  std::map<Ipv4Address, RoutingTableEntry> table = m_ipv4AddressEntry;
  Purge (table);
  std::ostream* os = stream->GetStream ();
  // Copy the current ostream state
  std::ios oldState (nullptr);
  oldState.copyfmt (*os);

  *os << std::resetiosflags (std::ios::adjustfield) << std::setiosflags (std::ios::left);
  *os << "\nAODV Routing table\n";
  *os << std::setw (16) << "Destination";
  *os << std::setw (16) << "Gateway";
  *os << std::setw (16) << "Interface";
  *os << std::setw (16) << "Flag";
  *os << std::setw (16) << "Expire";
  *os << "Hops" << std::endl;
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i =
         table.begin (); i != table.end (); ++i)
    {
      i->second.Print (stream, unit);
    }
  *stream->GetStream () << "\n";
}

}
}