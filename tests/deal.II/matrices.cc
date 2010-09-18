//----------------------------  matrices.cc  ---------------------------
//    $Id$
//    Version: $Name$ 
//
//    Copyright (C) 2000, 2001, 2003, 2004, 2007, 2008 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  matrices.cc  ---------------------------


/* Author: Wolfgang Bangerth, University of Heidelberg, 2001 */



#include "../tests.h"
#include <base/quadrature_lib.h>
#include <base/logstream.h>
#include <base/function_lib.h>
#include <lac/sparse_matrix.h>
#include <lac/vector.h>
#include <grid/tria.h>
#include <grid/tria_iterator.h>
#include <grid/tria_accessor.h>
#include <grid/grid_generator.h>
#include <dofs/dof_handler.h>
#include <dofs/dof_tools.h>
#include <lac/constraint_matrix.h>
#include <fe/fe_q.h>
#include <fe/fe_system.h>
#include <fe/mapping_q.h>
#include <numerics/matrices.h>

#include <fstream>


template<int dim>
class MySquareFunction : public Function<dim>
{
  public:
    MySquareFunction () : Function<dim>(2) {}
    
    virtual double value (const Point<dim>   &p,
			  const unsigned int  component) const
      {	return (component+1)*p.square(); }
    
    virtual void   vector_value (const Point<dim>   &p,
				 Vector<double>     &values) const
      { values(0) = value(p,0);
	values(1) = value(p,1); }
};




template <int dim>
void
check_boundary (const DoFHandler<dim> &dof,
		const Mapping<dim>    &mapping)
{
  MySquareFunction<dim> coefficient;
  typename FunctionMap<dim>::type function_map;
  function_map[0] = &coefficient;

  QGauss6<dim-1> face_quadrature;
  
  std::vector<unsigned int> dof_to_boundary_mapping;
  DoFTools::map_dof_to_boundary_indices (dof,
					 dof_to_boundary_mapping);

  SparsityPattern sparsity(dof.n_boundary_dofs(function_map),
			   dof.max_couplings_between_boundary_dofs());
  DoFTools::make_boundary_sparsity_pattern (dof,
					    function_map,
					    dof_to_boundary_mapping,
					    sparsity);
  sparsity.compress ();

  SparseMatrix<double> matrix;
  matrix.reinit (sparsity);
  
  Vector<double> rhs (dof.n_boundary_dofs(function_map));
  MatrixTools::
    create_boundary_mass_matrix (mapping, dof,
				 face_quadrature, matrix,
				 function_map, rhs,
				 dof_to_boundary_mapping,
				 &coefficient);

				   // since we only generate
				   // output with two digits after
				   // the dot, and since matrix
				   // entries are usually in the
				   // range of 1 or below,
				   // multiply matrix by 100 to
				   // make test more sensitive
  for (unsigned int i=0; i<matrix.n_nonzero_elements(); ++i)
    matrix.global_entry(i) *= 100;
  
				   // finally write out matrix
  matrix.print (deallog.get_file_stream());
}



void
check_boundary (const DoFHandler<1> &,
		const Mapping<1>    &)
{}




template <int dim>
void
check ()
{
  Triangulation<dim> tr;  
  if (dim==2)
    GridGenerator::hyper_ball(tr, Point<dim>(), 1);
  else
    GridGenerator::hyper_cube(tr, -1,1);
  tr.refine_global (1);
  tr.begin_active()->set_refine_flag ();
  tr.execute_coarsening_and_refinement ();
  if (dim==1)
    tr.refine_global(2);

				   // create a system element composed
				   // of one Q1 and one Q2 element
  FESystem<dim> element(FE_Q<dim>(1), 1,
			FE_Q<dim>(2), 1);
  DoFHandler<dim> dof(tr);
  dof.distribute_dofs(element);

				   // use a more complicated mapping
				   // of the domain and a quadrature
				   // formula suited to the elements
				   // we have here
  MappingQ<dim> mapping (3);
  QGauss<dim> quadrature(6);

				   // create sparsity pattern. note
				   // that different components should
				   // not couple, so use pattern
  SparsityPattern sparsity (dof.n_dofs(), dof.n_dofs());
  std::vector<std::vector<bool> > mask (2, std::vector<bool>(2, false));
  mask[0][0] = mask[1][1] = true;
  DoFTools::make_sparsity_pattern (dof, mask, sparsity);
  ConstraintMatrix constraints;
  DoFTools::make_hanging_node_constraints (dof, constraints);
  constraints.close ();
  constraints.condense (sparsity);
  sparsity.compress ();
  
  SparseMatrix<double> matrix;

  Functions::ExpFunction<dim> coefficient;
  
  typename FunctionMap<dim>::type function_map;
  function_map[0] = &coefficient;

  for (unsigned int test=0; test<2; ++test)
    {
      matrix.reinit(sparsity);
      switch (test)
	{
	  case 0:
		MatrixTools::
		  create_mass_matrix (mapping, dof,
				      quadrature, matrix, &coefficient);
		break;
	  case 1:
		MatrixTools::
		  create_laplace_matrix (mapping, dof,
					 quadrature, matrix, &coefficient);
		break;
	  default:
		Assert (false, ExcInternalError());
	};

				       // since we only generate
				       // output with two digits after
				       // the dot, and since matrix
				       // entries are usually in the
				       // range of 1 or below,
				       // multiply matrix by 100 to
				       // make test more sensitive
      for (unsigned int i=0; i<matrix.n_nonzero_elements(); ++i)
	deallog.get_file_stream() << matrix.global_entry(i) * 100
				  << std::endl;
    };

  if (dim > 1)
    check_boundary (dof, mapping);
}



int main ()
{
  std::ofstream logfile ("matrices/output");
  logfile << std::setprecision (2);
  logfile << std::fixed;  
  deallog << std::setprecision (2);
  deallog << std::fixed;  
  deallog.attach(logfile);
  deallog.depth_console (0);

  deallog.push ("1d");
  check<1> ();
  deallog.pop ();
  deallog.push ("2d");
  check<2> ();
  deallog.pop ();
  deallog.push ("3d");
  check<3> ();
  deallog.pop ();
}
