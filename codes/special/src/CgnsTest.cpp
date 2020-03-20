/*-----------------------this->----------------------------------------------------*\
    OneFLOW - LargeScale Multiphysics Scientific Simulation Environment
    Copyright (C) 2017-2020 He Xin and the OneFLOW contributors.
-------------------------------------------------------------------------------
License
    This file is part of OneFLOW.

    OneFLOW is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OneFLOW is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OneFLOW.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "CgnsTest.h"
#include "CgnsFile.h"
#include "CgnsBase.h"
#include "CgnsFactory.h"
#include "Prj.h"
#include "StrUtil.h"
#include "CgnsZone.h"
#include <iostream>
#include <iomanip>
using namespace std;

BeginNameSpace( ONEFLOW )

CgnsTest::CgnsTest()
{
    ;
}

CgnsTest::~CgnsTest()
{
    ;
}

void CgnsTest::Init()
{
    this->SetDefaultGridName();
}

void CgnsTest::Run()
{
    this->Init();

    //this->WriteSimpleMultiBaseTest();
    //this->ReadSimpleMultiBaseTest();
    //this->WriteEmptyCgnsFile();
    //this->ReadEmptyCgnsFile();
    //this->WriteDescriptor();
    this->ReadDescriptor();
    //this->Test();
    //this->TestCgnsLink();
    //this->WriteArray();
    //this->ReadArray();
    //this->WriteReferenceState();
    //this->ReadReferenceState();
}

void CgnsTest::Test()
{
   this->TestCgnsLink();
}

void CgnsTest::SetDefaultGridName()
{
    string gridName = "/grid/oneflow.cgns";
    string prjFileName = ONEFLOW::GetPrjFileName( gridName );
    cout << " CGNS File Name = " << prjFileName << "\n";

    this->fileName = prjFileName;
}

void CgnsTest::WriteSimpleMultiBaseTest()
{
    CgnsFile * cgnsFile = new CgnsFile( "smiplebase.cgns", CG_MODE_WRITE );
    cgnsFile->WriteBase( "OneFLOW1" );
    cgnsFile->WriteBase( "OneFLOW 2" );
    cgnsFile->WriteBase( "CGNS base 3" );
    cgnsFile->WriteBase( "Fluid" );
    cgnsFile->WriteBase( "CAE library" );
    delete cgnsFile;
}

void CgnsTest::ReadSimpleMultiBaseTest()
{
    CgnsFile * cgnsFile = new CgnsFile( "smiplebase.cgns", CG_MODE_READ );
    cgnsFile->ReadBases();
    delete cgnsFile;
}

void CgnsTest::WriteDescriptor()
{
    CgnsFile * cgnsFile = new CgnsFile( "descript.cgns", CG_MODE_WRITE );
    cgnsFile->WriteBaseDescriptor();
    delete cgnsFile;
}

void CgnsTest::ReadDescriptor()
{
    CgnsFile * cgnsFile = new CgnsFile( "descript.cgns", CG_MODE_READ );
    cgnsFile->ReadBaseDescriptor();
    delete cgnsFile;
}

void CgnsTest::WriteEmptyCgnsFile()
{
    CgnsFile * cgnsFile = new CgnsFile( "empty.cgns", CG_MODE_WRITE );
    delete cgnsFile;
}

void CgnsTest::ReadEmptyCgnsFile()
{
    CgnsFile * cgnsFile = new CgnsFile( "empty.cgns", CG_MODE_READ );
    delete cgnsFile;
}

void CgnsTest::WriteDouble( const string & varName, const double & varValue )
{
    int nDim = 1;
    cgsize_t ndims[ 1 ] = { 1 };
    cg_array_write( varName.c_str(), CGNS_ENUMV(RealDouble), nDim ,ndims, &varValue );
}

void CgnsTest::SetISize( cgsize_t * isize )
{
    int nijk = 5;
    for ( int n = 0; n < 3; n ++ )
    {
        isize[ n     ] = nijk;
        isize[ n + 3 ] = nijk - 1;
        isize[ n + 6 ] = 0;
    }
}


void CgnsTest::TestCgnsLink()
{
    string fname    = "zones.cgns";
    string linkname = "zones_link.cgns";

    cgsize_t isize[ 9 ];
    this->SetISize( isize );
    int nZones = 5;

    CgnsFile * fileZone = new CgnsFile( fname, CG_MODE_WRITE );
    CgnsBase * cgnsBase = fileZone->WriteBase( "Base" );

    for ( int iZone = 0; iZone < nZones; ++ iZone )
    {
        string name = AddString( "Zone", iZone + 1 );
        cgnsBase->WriteZoneInfo( name, CGNS_ENUMV( Structured ), isize );
    }
    delete fileZone;

    CgnsFile * fileZoneM = new CgnsFile( fname, CG_MODE_MODIFY );
    CgnsBase * cgnsBaseM = fileZoneM->WriteBase( "Base" );

    for ( int iZone = 0; iZone < nZones; ++ iZone )
    {
        string name = AddString( "Zone", iZone + 1 );
        cgnsBaseM->WriteZoneInfo( name, CGNS_ENUMV( Structured ), isize );
    }

    delete fileZoneM;

    CgnsFile * fileLink = new CgnsFile( linkname, CG_MODE_WRITE );
    CgnsBase * cgnsBaseLink = fileLink->WriteBase( "Base2" );
    cgnsBaseLink->GoToBase();

    for ( int iZone = 0; iZone < nZones; ++ iZone )
    {
        string name     = AddString( "Link to Zone", iZone + 1 );
        string linkpath = AddString( "/Base/Zone", iZone + 1 );

        cg_link_write( name.c_str(), fname.c_str(), linkpath.c_str() );
    }

    delete fileLink;
}

void CgnsTest::GetArray( vector< vector< float > > & myfloat2d )
{
    vector< float > a1, a2, a3;
    a1.push_back( 1 );
    a1.push_back( 2 );
    a1.push_back( 3 );

    a2.push_back( 10 );
    a2.push_back( 20 );
    a2.push_back( 30 );
    a2.push_back( 40 );

    a3.push_back( 100 );
    a3.push_back( 200 );
    a3.push_back( 300 );
    a3.push_back( 400 );
    a3.push_back( 500 );

    myfloat2d.push_back( a1 );
    myfloat2d.push_back( a2 );
    myfloat2d.push_back( a3 );
}

void CgnsTest::WriteArray()
{
    vector< vector< float > > myarray;
    this->GetArray( myarray );

    CgnsFile * cgnsFile = new CgnsFile( "array.cgns", CG_MODE_WRITE );
    CgnsBase * cgnsBase = cgnsFile->WriteBase( "BaseXXX" );
    this->WriteArray( cgnsFile, cgnsBase );
    cgnsBase = cgnsFile->WriteBase( "BaseYYY" );
    this->WriteArray( cgnsFile, cgnsBase );

    delete cgnsFile;
}

void CgnsTest::WriteArray( CgnsFile * cgnsFile, CgnsBase * cgnsBase )
{
    vector< vector< float > > myarray;
    this->GetArray( myarray );

    cgnsBase->GoToBase();
    cg_user_data_write( "DataYYY" );
    cgnsBase->GoToNode( "UserDefinedData_t", 1 );

    for ( int i = 0; i < myarray.size(); ++ i )
    {
        string name = AddString( "MyArray", i + 1 );
        cgsize_t arraysize = myarray[ i ].size();
        cg_array_write( name.c_str(), CGNS_ENUMV( RealSingle ), 1, &arraysize, &myarray[ i ][ 0 ] );
        cgnsFile->GoPath( name );
        cgnsFile->GoPath( ".." );
    }

    cgnsBase->GoToBase();
    cg_user_data_write( "DataZZZ" );
    cgnsBase->GoToNode( "UserDefinedData_t", 2 );

    for ( int i = 0; i < myarray.size(); ++ i )
    {
        string name = AddString( "MyArray", i + 1 );
        cgsize_t arraysize = myarray[ i ].size();
        cg_array_write( name.c_str(), CGNS_ENUMV( RealSingle ), 1, &arraysize, &myarray[ i ][ 0 ] );
        cgnsFile->GoPath( name );
        cgnsFile->GoPath( ".." );
    }
}

void CgnsTest::ReadArray()
{
    CgnsFile * cgnsFile = new CgnsFile( "array.cgns", CG_MODE_READ );
    cgnsFile->ReadArray();
    delete cgnsFile;
}

void CgnsTest::WriteReferenceState()
{
    //define nondimensional parameters
    double xmach    = 4.6;
    double reue     = 6000000.;
    double xmv      = xmach;
    double xmc      = 1.;
    double rev      = xmach;
    double rel      = 1.;
    double renu     = xmach / reue;
    double rho0     = 1.;
    double gamma    = 1.4;
    double p0       = 1./gamma;
    double c0       = 1.;
    double vm0      = xmach/reue;
    double xlength0 = 1.;
    double vx       = xmach;
    double vy       = 0.0;
    double vz       = 0.0;

    CgnsFile * cgnsFile = new CgnsFile( "refstate.cgns", CG_MODE_WRITE );
    CgnsBase * cgnsBase1 = cgnsFile->WriteBase( "Base1" );

    cgnsBase1->GoToBase();
    cg_dataclass_write(CGNS_ENUMV(NormalizedByUnknownDimensional));
    cg_state_write("ReferenceQuantities");
    cgnsBase1->GoToNode( "ReferenceState_t", 1 );

    WriteDouble("Mach", xmach );
    WriteDouble("Reynolds", reue );

    WriteDouble("Mach_Velocity", xmv );
    WriteDouble("Mach_VelocitySound", xmc );
    WriteDouble("Reynolds_Velocity", rev );
    WriteDouble("Reynolds_Length", rel );
    WriteDouble("Reynolds_ViscosityKinematic", renu );
    
    //Next, write flow field reference quantities:
    WriteDouble("Density", rho0 );
    WriteDouble("Pressure", p0 );
    WriteDouble("VelocitySound", c0 );
    WriteDouble("ViscosityMolecular", vm0 );
    WriteDouble("LengthReference", xlength0 );

    CgnsBase * cgnsBase2 = cgnsFile->WriteBase( "Base2" );
    cgnsBase2->GoToBase();
    cg_dataclass_write(CGNS_ENUMV(NormalizedByUnknownDimensional));
    cg_state_write("Test1");
    cgnsBase1->GoToNode( "ReferenceState_t", 1 );
    WriteDouble("Mach", xmach );
    WriteDouble("Reynolds", reue );

    CgnsBase * cgnsBase3 = cgnsFile->WriteBase( "Base3" );
    cgnsBase3->GoToBase();
    cg_state_write("Test2");

    delete cgnsFile; 
}

void CgnsTest::ReadReferenceState()
{
    CgnsFile * cgnsFile = new CgnsFile( "refstate.cgns", CG_MODE_READ );
    cgnsFile->ReadReferenceState();
    delete cgnsFile; 
}

EndNameSpace