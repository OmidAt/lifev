//@HEADER
/*
*******************************************************************************

Copyright (C) 2004, 2005, 2007 EPFL, Politecnico di Milano, INRIA
Copyright (C) 2010 EPFL, Politecnico di Milano, Emory University

This file is part of LifeV.

LifeV is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LifeV is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with LifeV.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************
*/
//@HEADER
/*!
  @file
  @brief exporter and ExporterData classes provide interfaces for post-processing

  @date 11-11-2008
  @author Simone Deparis <simone.deparis.epfl.ch>

  @maintainer Radu Popescu <radu.popescu@epfl.ch>

    Usage: two steps
    <ol>
        <li> first: add the variables using addVariable
        <li> second: call postProcess( time );
    </ol>
*/

#ifndef EXPORTER_H
#define EXPORTER_H 1

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <boost/shared_ptr.hpp>

#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-parameter"

#include <life/lifearray/EpetraVector.hpp>
#include <life/lifefilters/GetPot.hpp>
#include <life/lifecore/LifeChrono.hpp>
#include <life/lifecore/life.hpp>
#include <life/lifefem/ReferenceFE.hpp>
#include <life/lifemesh/markers.hpp>

namespace LifeV
{

//! ExporterData - Holds the datastructure of the an array to import/export
/*!
    @author Simone Deparis <simone.deparis@epfl.ch>
    @date 11-11-2008

    This class holds the datastructure of one datum
    to help the importer/exporter
    like:   variable name, shared pointer to data, size, and few others.
 */
class ExporterData
{
public:

    //! @name Public Types
    //@{

    typedef EpetraVector                      vectorRaw_Type;
    typedef boost::shared_ptr<vectorRaw_Type> vectorPtr_Type;

    //! Type of data stored.
    enum Type
    {
        Scalar, /*!< Scalar stands for scalarfield */
        Vector  /*!< Vector stands for vectorfield */
    };

    //! Where is data centered /
    enum Where
    {
        Node, /*!< Node centered */
        Cell  /*!< Cell centered */
    };

    //@}

    //! @name Constructor & Destructor
    //@{

    //! Constructor with all the data to be stored
    /*!
        Constructor with all the data to be stored
        @param type         - scalar or vector field
        @param variableName - name assigned to this variable in output file
        @param vec          - shared pointer to array
        @param start        - address of first datum in the array to be stored.
                              Useful in case you want to define a subrange of the vector *vec
        @param size         - size of the array to store (not yet multiplied by the dimension of the vector)
        @param steady       - if  file name for postprocessing has to include time dependency
        @param where        - where is variable located (Node or Cell)
    */
    ExporterData(const Type&           type,
                 const std::string&    variableName,
                 const vectorPtr_Type& vec,
                 const UInt&           start,
                 const UInt&           size,
                 const UInt&           steady,
                 const Where&          where = ExporterData::Node);

    //@}

    //! @name Operators
    //@{

    //! Accessor to component (const)
    /*!
        @param i which component to access
    */
    Real operator()(const UInt i) const;

    //! Accessor to component (reference)
    /*!
        @param i which component to access
    */
    Real& operator()(const UInt i);

    //@}

    //! @name Set Methods
    //@{

    //! file name for postprocessing has to include time dependency
    void setSteady(UInt i) {M_steady = i;}

    //@}


    //! @name Get Methods
    //@{

    //! name assigned to this variable in output file
    const std::string&  variableName() const
    {
        return M_variableName;
    }

    //! size of the stored array
    const UInt& size() const
    {
        return M_size;
    }

    //! address of first datum in the array
    const UInt& start() const { return M_start; }

    //! scalar or vector field
    const Type& type() const
    {
        return M_type;
    }

    //! shared pointer to array
    const vectorPtr_Type storedArray() const
    {
        return M_vr;
    }

    //! returns 0 if file name for postprocessing has to include time dependency
    UInt steady() const {return M_steady; }

    //! returns Scalar or Vector strings
    std::string typeName() const;

    //! returns 1 (if Scalar) or 3 (if Vector)
    UInt typeDim() const;

    //! Node or Cell centered ?
    const Where& where() const
    {
        return M_where;
    }

    //! returns Node or Cell centered string
    std::string whereName() const;
    //@}

private:
    //! @name Private data members
    //@{
    //! name assigned to this variable in output file
    std::string M_variableName;

    //! pointer to storedArray
    const vectorPtr_Type M_vr;

    //! address of first datum in the array
    UInt M_size;

    //! address of first datum in the array
    UInt M_start;

    //! scalar or vector field
    Type M_type;

    //! equal to 0 if file name for postprocessing has to include time dependency
    UInt M_steady;

    //! Node or Cell centered
    Where M_where;
    //@}
};


//! Exporter - Pure virtual class that describes a generic exporter
/*!
    @author Simone Deparis <simone.deparis@epfl.ch>
    @date 11-11-2008

    This class is pure virtual and describes a generic exporter that can
    also do import
 */
template<typename MeshType>
class Exporter
{

public:
    //! @name Public typedefs
    //@{
    typedef MeshType mesh_Type;
    typedef boost::shared_ptr<MeshType>    meshPtr_Type;
    typedef ExporterData::vectorRaw_Type vectorRaw_Type;
    typedef ExporterData::vectorPtr_Type vectorPtr_Type;
    //@}

    //! @name Constructor & Destructor
    //@{

    //! Empty constructor for Exporter
    Exporter();

    //! Constructor for Exporter without prefix and procID
    /*!
        In this case prefix and procID should be set separately
        @param dfile the GetPot data file where you must provide an [exporter] section with:
          "start"     (start index for filenames 0 for 000, 1 for 001 etc.),
          "save"      (how many time steps per postprocessing)
          "multimesh" ( = true if the mesh has to be saved at each post-processing step)
       @param the prefix for the case file (ex. "test" for test.case)
    */
    Exporter(const GetPot& dfile, const std::string& prefix);

    //! Destructor
    virtual ~Exporter() {};
    //@}


    //! @name Methods
    //@{

    //! Adds a new variable to be post-processed
    /*!
        @param type the type fo the variable Ensight::Scalar or Ensight::Vector
        @param prefix the prefix of the files storing the variable (ex: "velocity" for velocity.***)
        @param vr an ublas::vector_range type given a view of the varialbe (ex: subrange(fluid.u(),0,3*dimU) )
        @param size size of the stored array
    */
    void addVariable(const ExporterData::Type& type,
                     const std::string& variableName,
                     const vectorPtr_Type& vector,
                     const UInt& start,
                     const UInt& size,
                     const UInt& steady = 0,
                     const ExporterData::Where& where = ExporterData::Node );

    //! Post-process the variables added to the list
    /*!
        @param time the solver time
    */
    virtual void postProcess(const Real& time) = 0;

    //! Import data from previous simulations at a certain time
    /*!
       @param Time the time of the data to be imported
     */
    virtual UInt importFromTime( const Real& Time ) = 0;

    //! Import data from previous simulations
    /*!
       @param time the solver time
       @param dt time step used to rebuild the history up to now
    */
    virtual void import(const Real& startTime, const Real& dt) = 0;

    //! Read  only last timestep
    virtual void import(const Real& startTime) = 0;

    virtual void readVariable(ExporterData& dvar);
    //@}

    //! @name Set Methods
    //@{

    //! Set data from file.
    /*!
     * @param dataFile data file.
     * @param section section in the data file.
     */
    void setDataFromGetPot( const GetPot& dataFile, const std::string& section = "exporter" );

    //! Set prefix.
    /*!
     * @param prefix prefix.
     */
    void setPrefix( const std::string& prefix )
    {
        M_prefix = prefix;
    }

    //! Set the folder for pre/postprocessing
    /*!
     * @param Directory output folder
     */
    void setPostDir( const std::string& Directory )
    {
        M_postDir = Directory;
    }

    //! Set the folder for pre/postprocessing
    /*!
     * @param Directory output folder
     */
    void setStartIndex( const UInt& StartIndex )
    {
        M_startIndex = StartIndex;
    }

    //! Set how many time step between two saves.
    /*!
     * @param save steps
     */
    void setSave( const UInt& save )
    {
        M_save = save;
    }

    //! Set if to save the mesh at each time step.
    /*!
     * @param multimesh multimesh
     */
    void setMultimesh( const bool& multimesh )
    {
        M_multimesh = multimesh;
    }

    virtual void setMeshProcId( const meshPtr_Type mesh, const int& procId );

    //! Close the output file
    /*!
         This method is only used by  some of the exporter which derived from this class.
     */
    virtual void closeFile() {}
    //@}

    //! @name Get Methods
    //@{

    const UInt& startIndex() const { return M_startIndex; }

    //! returns the type of the map to use for the EpetraVector
    virtual EpetraMapType mapType() const = 0;
    //@}

protected:

    //! @name Protected methods
    //@{
    //! compute postfix
    void computePostfix();

    virtual void readScalar( ExporterData& dvar ) = 0;
    virtual void readVector( ExporterData& dvar ) = 0;

    //@}

    //! @name Protected data members
    //@{
    std::string                 M_prefix;
    std::string                 M_postDir;
    UInt                        M_startIndex;
    UInt                        M_save;
    bool                        M_multimesh;
    meshPtr_Type                M_mesh;
    int                         M_procId;
    std::string                 M_postfix;

    std::list<ExporterData>     M_listData;
    std::list<Real>             M_timeSteps;
    //@}
};

// ==================================================
// IMPLEMENTATION
// ==================================================

// ===================================================
// Constructors
// ===================================================
template<typename MeshType>
Exporter<MeshType>::Exporter():
        M_prefix        ( "output"),
        M_postDir      ( "./" ),
        M_startIndex         ( 0 ),
        M_save          ( 1 ),
        M_multimesh     ( true )
{}

template<typename MeshType>
Exporter<MeshType>::Exporter( const GetPot& dfile, const std::string& prefix ):
        M_prefix        ( prefix ),
        M_postDir      ( dfile("exporter/post_dir", "./") ),
        M_startIndex         ( dfile("exporter/start",0) ),
        M_save          ( dfile("exporter/save",1) ),
        M_multimesh     ( dfile("exporter/multimesh",true) )
{}

// ===================================================
// Methods
// ===================================================
template<typename MeshType>
void Exporter<MeshType>::addVariable(const ExporterData::Type&  type,
                                 const std::string&         variableName,
                                 const vectorPtr_Type&      vr,
                                 const UInt&                start,
                                 const UInt&                size,
                                 const UInt&                steady,
                                 const ExporterData::Where& where)
{
    M_listData.push_back( ExporterData(type,variableName, vr, start, size, steady, where) );
}

template <typename MeshType>
void Exporter<MeshType>::readVariable(ExporterData& dvar)
{
    switch ( dvar.type() )
    {
    case ExporterData::Scalar:
        readScalar(dvar);
        break;
    case ExporterData::Vector:
        readVector(dvar);
        break;
    }
}

template <typename MeshType>
void Exporter<MeshType>::computePostfix()
{
    std::ostringstream index;
    index.fill( '0' );

    if (M_startIndex % M_save == 0)
    {
        index << std::setw(5) << ( M_startIndex / M_save );

        M_postfix = "." + index.str();
    }
    else
        M_postfix = "*****";

    ++M_startIndex;
}

// ===================================================
// Set Methods
// ===================================================
template<typename MeshType>
void Exporter<MeshType>::setDataFromGetPot( const GetPot& dataFile, const std::string& section )
{
    M_postDir      = dataFile( ( section + "/post_dir"  ).data(), "./" );
    M_startIndex   = dataFile( ( section + "/start"     ).data(), 0 );
    M_save          = dataFile( ( section + "/save"      ).data(), 1 );
    M_multimesh     = dataFile( ( section + "/multimesh" ).data(), true );
}

template<typename MeshType>
void Exporter<MeshType>::setMeshProcId( const meshPtr_Type mesh , const int& procId )
{
    M_mesh   = mesh;
    M_procId = procId;
}

} // Namespace LifeV

#endif // EXPORTER_H