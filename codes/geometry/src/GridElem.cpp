/*---------------------------------------------------------------------------*\
    OneFLOW - LargeScale Multiphysics Scientific Simulation Environment
    Copyright (C) 2017-2021 He Xin and the OneFLOW contributors.
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

#include "GridElem.h"
#include "CgnsZone.h"
#include "CgnsZbase.h"
#include "GridPara.h"
#include "HXCgns.h"
#include "UnsGrid.h"
#include "HXMath.h"
#include "CellTopo.h"
#include "CellMesh.h"
#include "ElemFeature.h"
#include "FaceTopo.h"
#include "FaceSolver.h"
#include "BcRecord.h"
#include "Boundary.h"
#include "PointFactory.h"
#include "NodeMesh.h"
#include "GridState.h"
#include "BgGrid.h"
#include <iostream>
#include <iomanip>
using namespace std;

BeginNameSpace( ONEFLOW )

int OneFlow2CgnsZoneType( int zoneType )
{
    if ( zoneType == UMESH )
    {
        return CGNS_ENUMV( Unstructured );
    }
    else
    {
        return CGNS_ENUMV( Structured );
    }
}

int Cgns2OneFlowZoneType( int zoneType )
{
    if ( zoneType == CGNS_ENUMV( Unstructured ) )
    {
        return UMESH;
    }
    else
    {
        return SMESH;
    }
}

GridElem::GridElem( HXVector< CgnsZone * > & cgnsZones, int iZone )
{
    this->cgnsZones = cgnsZones;
    this->CreateGrid( cgnsZones, iZone );

    this->minLen = LARGE;
    this->maxLen = -LARGE;

    this->delFlag = false;

    this->point_factory = new PointFactory();
    this->elem_feature = new ElemFeature();
    this->face_solver = new FaceSolver();
    this->elem_feature->face_solver = face_solver;
}

GridElem::~GridElem()
{
    delete this->point_factory;
    delete this->elem_feature;
    delete this->face_solver;
    if ( this->delFlag )
    {
        delete this->grid;
    }
}

CgnsZone * GridElem::GetCgnsZone( int iZone )
{
    return this->cgnsZones[ iZone ];
}

int GridElem::GetNZones()
{
    return this->cgnsZones.size();
}

void GridElem::CreateGrid( HXVector< CgnsZone * > cgnsZones, int iZone )
{
    CgnsZone * cgnsZone = cgnsZones[ 0 ];
    int cgnsZoneType = cgnsZone->cgnsZoneType;
    int gridType = Cgns2OneFlowZoneType( cgnsZoneType );
    this->grid = ONEFLOW::CreateGrid( gridType );
    grid->level = 0;
    grid->id = iZone;
    grid->localId = iZone;
    grid->type = gridType;
    grid->volBcType = cgnsZone->GetVolBcType();
}

void GridElem::PrepareUnsCalcGrid()
{
    cout << " InitCgnsElements()\n";
    this->InitCgnsElements();
    cout << " ScanElements()\n";
    this->elem_feature->ScanElements();
    cout << " ScanBcFace()\n";
    this->ScanBcFace();

    //Continue to parse
    cout << " ScanElements()\n";
    this->elem_feature->ScanElements();
    this->GenerateCalcElement();
}

void GridElem::InitCgnsElements()
{
    int nZone = this->GetNZones();
    for ( int iZone = 0; iZone < nZone; ++ iZone )
    {
        CgnsZone * cgnsZone = this->GetCgnsZone( iZone );
        
        cgnsZone->InitElement( this );
    }
}

void GridElem::ScanBcFace()
{
    int nZone = this->GetNZones();
    for ( int iZone = 0; iZone < nZone; ++ iZone )
    {
        CgnsZone * cgnsZone = this->GetCgnsZone( iZone );
        cgnsZone->ScanBcFace( this->elem_feature->face_solver );
    }

    this->elem_feature->face_solver->ScanInterfaceBc();
}

void GridElem::GenerateCalcElement()
{
    int nElement =  this->elem_feature->eTypes->size();

    FaceTopo * faceTopo = this->face_solver->faceTopo;

    int nFace = this->face_solver->faceTopo->faces.size();

    int nBFace = 0;

    //cout << " nFace = " << nFace << "\n";

    for ( int iFace = 0; iFace < nFace; ++ iFace )
    {
        if ( iFace % 200000 == 0 ) 
        {
            cout << " iFace = " << iFace << " numberOfTotalFaces = " << nFace << endl;
        }

        int rc = ( faceTopo->rCell )[ iFace ];

        if ( rc == INVALID_INDEX )
        {
            faceTopo->bcManager->bcRecord->bcType.push_back( ( * this->face_solver->faceBcType )[ iFace ] );
            faceTopo->bcManager->bcRecord->bcNameId.push_back( ( * this->face_solver->faceBcKey )[ iFace ] );
            ++ nBFace;
        }
    }

    this->point_factory->InitC2g();

}

void GridElem::GenerateCalcGrid()
{
    this->GenerateCalcGrid( this->grid );
}

void GridElem::GenerateCalcGrid( Grid * gridIn )
{
    UnsGrid * grid = UnsGridCast ( gridIn );

    grid->nCell = this->elem_feature->eTypes->size();
    grid->cellMesh->cellTopo->eTypes = * this->elem_feature->eTypes;
    cout << "   nCell = " << grid->nCell << endl;

    int nNodes = this->point_factory->c2g.size();
    grid->nodeMesh->CreateNodes( nNodes );
    grid->nNodes = nNodes;

    for ( int iNode = 0; iNode < nNodes; ++ iNode )
    {
        int nodeIndex = this->point_factory->c2g[ iNode ];

        grid->nodeMesh->xN[ iNode ] = this->point_factory->pointList[ nodeIndex ].x;
        grid->nodeMesh->yN[ iNode ] = this->point_factory->pointList[ nodeIndex ].y;
        grid->nodeMesh->zN[ iNode ] = this->point_factory->pointList[ nodeIndex ].z;
    }

    this->CalcBoundaryType( grid );
    this->ReorderLink( grid );

    cout << "\n-->All the computing information is ready\n";
}

void GridElem::CalcBoundaryType( UnsGrid * grid )
{
    cout << "\n-->Set boundary condition......\n";
    delete grid->faceTopo;
    grid->faceTopo = this->face_solver->faceTopo;
    grid->faceTopo->grid = grid;
    this->face_solver->faceTopo = 0;
    int nFace = grid->faceTopo->faces.size();
    cout << " nFace = " << nFace << "\n";
     
    BcRecord * bcRecord = grid->faceTopo->bcManager->bcRecord;
    int nBFace = bcRecord->bcType.size();

    grid->nBFace = nBFace;

    cout << " nBFace = " << nBFace << "\n";

    BcTypeMap * bcTypeMap = new BcTypeMap();
    bcTypeMap->Init();

    IntField cgnsBcArray = bcRecord->bcType;

    IntSet originalBcSet, finalBcSet;
    int iCount = 0;
    for ( int iFace = 0; iFace < nBFace; ++ iFace )
    {
        int cgnsBcType = bcRecord->bcType[ iFace ];
        int bcNameId = bcRecord->bcNameId[ iFace ];
        int bcType = bcTypeMap->Cgns2OneFlow( cgnsBcType );

        bcRecord->bcType[ iCount ] = bcType;

        originalBcSet.insert( cgnsBcType );
        finalBcSet.insert( bcType );
        ++ iCount;
    }

    delete bcTypeMap;

    IntField nBFaceSub;

    for ( IntSet::iterator iter = originalBcSet.begin(); iter != originalBcSet.end(); ++ iter )
    {
        int iCount = 0;
        for ( int iFace = 0; iFace < nBFace; ++ iFace )
        {
            int cgnsBcType = cgnsBcArray[ iFace ];
            if ( cgnsBcType == * iter )
            {
                ++ iCount;
            }
        }
        nBFaceSub.push_back( iCount );
    }

    cout << " Original Boundary Condition Number is " << originalBcSet.size() << endl;
    iCount = 0;
    for ( IntSet::iterator iter = originalBcSet.begin(); iter != originalBcSet.end(); ++ iter )
    {
        int oriBcType = * iter;
        cout << " Boundary Type = " << setw( 3 ) << oriBcType << "  Name = " << setiosflags(ios::left) << setw( 23 ) << GetCgnsBcName( oriBcType );
        cout << " Face = " << nBFaceSub[ iCount ] << endl;
        ++ iCount;
    }
    cout << endl;
    cout << " Final Boundary Condition Number is " << finalBcSet.size() << endl;
    cout << " Boundary Type : ";
    for ( IntSet::iterator iter = finalBcSet.begin(); iter != finalBcSet.end(); ++ iter )
    {
        cout << * iter << " ";
    }
    cout << endl;
}

void GridElem::ReorderLink( UnsGrid * grid )
{
    FaceTopo * faceTopo = grid->faceTopo;
    int nFace = faceTopo->fTypes.size();
    grid->nFace = nFace;

    IntField f1map( nFace ), f2map( nFace );
    int iCount = 0;
    for ( int iFace = 0; iFace < nFace; ++ iFace )
    {
        int rc = faceTopo->rCell[ iFace ];
        if ( rc == INVALID_INDEX )
        {
            f1map[ iFace ] = iCount;
            f2map[ iCount ] = iFace;
            ++ iCount;
        }
    }

    for ( int iFace = 0; iFace < nFace; ++ iFace )
    {
        int rc = faceTopo->rCell[ iFace ];
        if ( rc != INVALID_INDEX )
        {
            f1map[ iFace ] = iCount;
            f2map[ iCount ] = iFace;
            ++ iCount;
        }
    }
    faceTopo->facesNew.resize( nFace );
    faceTopo->lCellNew.resize( nFace );
    faceTopo->rCellNew.resize( nFace );
    for ( int iFace = 0; iFace < nFace; ++ iFace )
    {
        int jFace = f2map[ iFace ];
        faceTopo->facesNew[ iFace ] = faceTopo->faces[ jFace ];
        faceTopo->lCellNew[ iFace ] = faceTopo->lCell[ jFace ];
        faceTopo->rCellNew[ iFace ] = faceTopo->rCell[ jFace ];
    }
    faceTopo->faces = faceTopo->facesNew;
    faceTopo->lCell = faceTopo->lCellNew;
    faceTopo->rCell = faceTopo->rCellNew;
}

ZgridElem::ZgridElem( CgnsZbase * cgnsZbase )
{
    this->cgnsZbase = cgnsZbase;
}

ZgridElem::~ZgridElem()
{
    for ( int i = 0; i < this->data.size(); ++ i )
    {
        delete this->data[ i ];
    }
}

void ZgridElem::AddGridElem( GridElem * gridElem )
{
    this->data.push_back( gridElem );
}

void ZgridElem::AddGridElem( HXVector< CgnsZone * > cgnsZones, int iZone )
{
    GridElem * gridElem = new GridElem( cgnsZones, iZone );
    this->AddGridElem( gridElem );
}

GridElem * ZgridElem::GetGridElem( int iGridElem )
{
    return this->data[ iGridElem ];
}

void ZgridElem::AllocateGridElem()
{
    if ( grid_para.multiBlock == 0 )
    {
        HXVector< CgnsZone * > cgnsZones;

        int nOriZone = cgnsZbase->GetNZones();

        for ( int iZone = 0; iZone < nOriZone; ++ iZone )
        {
            cgnsZones.push_back( cgnsZbase->GetCgnsZone( iZone ) );
        }

        int nZones = 1;

        for ( int iZone = 0; iZone < nZones; ++ iZone )
        {
            this->AddGridElem( cgnsZones, iZone );
        }

    }
    else
    {
        int nZones = cgnsZbase->GetNZones();

        for ( int iZone = 0; iZone < nZones; ++ iZone )
        {
            HXVector< CgnsZone * > cgnsZones;
            cgnsZones.push_back( cgnsZbase->GetCgnsZone( iZone ) );

            this->AddGridElem( cgnsZones, iZone );
        }
    }
}

void ZgridElem::PrepareUnsCalcGrid()
{
    int nZones = this->data.size();
    for ( int iZone = 0; iZone < nZones; ++ iZone )
    {
        GridElem * gridElem = this->GetGridElem( iZone );
        gridElem->PrepareUnsCalcGrid();
    }
}

void ZgridElem::GenerateCalcGrid()
{
    int nZones = this->data.size();
    for ( int iZone = 0; iZone < nZones; ++ iZone )
    {
        GridElem * gridElem = this->GetGridElem( iZone );
        gridElem->GenerateCalcGrid();
    }
}

void ZgridElem::GetGrids( Grids & grids )
{
    int nZones = this->data.size();
    for ( int iZone = 0; iZone < nZones; ++ iZone )
    {
        GridElem * gridElem = this->GetGridElem( iZone );
        Grid * grid = gridElem->grid;
        grids.push_back( grid );
    }
}

void ZgridElem::GenerateLocalOneFlowGrid( Grids & grids )
{
    this->AllocateGridElem();

    this->PrepareUnsCalcGrid();

    this->GenerateCalcGrid();

    this->GetGrids( grids );
}


EndNameSpace